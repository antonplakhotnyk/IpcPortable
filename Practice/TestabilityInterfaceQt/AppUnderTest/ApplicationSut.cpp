#include "ApplicationSut.h"
#include "IpcQt_Serializers.h"
#include "IpcQt_Global.h"


std::shared_ptr<ApplicationSut> ApplicationSut::m_int;

ApplicationSut::ApplicationSut(const Ipc::Addr& connect_to_address)
	:m_testability(Ipc::Inst(), connect_to_address)
{

	{
		Ipc::Inst().AddHandler("AutotestServerReady", std::function<void()>([this]()
		{
			m_server_ready_request_pending = true;
			AutotestServerReady();
		}), {}, IpcQt_TransthreadCaller::GetCurrentThreadId(), this);
	}

	m_testability.Start();
}

void ApplicationSut::AutotestServerReady()
{
	if( m_server_ready_request_pending )
	{
		m_server_ready_request_pending = false;
		Ipc::Inst().AsyncInvoke("SutReady");
	}
}

ApplicationSut::~ApplicationSut()
{
	Unregister(m_score_component);
}

void ApplicationSut::ApplicationUnderTest_Register(ApplicationUnderTest* score_component)
{
	mass_return_if_equal(bool(m_int), false);
	m_int->Register(score_component);
}

void ApplicationSut::Register(ApplicationUnderTest* score_component)
{
	Unregister(m_score_component);
	m_score_component = score_component;

	QObject::connect(score_component, &QObject::destroyed, m_int.get(), &ApplicationSut::Unregister);

	MAssIpcCall& ipc = Ipc::Inst();

	ipc.AddHandler("OpenFile", std::function<bool(QByteArray)>(std::bind(&ApplicationSut::OpenFile, this, std::placeholders::_1)), {}, IpcQt_TransthreadCaller::GetCurrentThreadId(), score_component);
	ipc.AddHandler("TransfetString", std::function<QString(QString)>(std::bind(&ApplicationSut::TransfetString, this, std::placeholders::_1)), {}, IpcQt_TransthreadCaller::GetCurrentThreadId(), score_component);

	AutotestServerReady();
}

void ApplicationSut::Unregister(QObject* obj)
{
	if( bool(m_score_component) && (m_score_component == obj) )
	{
		Ipc::Inst().ClearHandlersWithTag(obj);
		m_score_component.clear();
	}
}

bool ApplicationSut::OpenFile(const QByteArray& file_data)
{
	mass_return_x_if_equal(bool(m_score_component), false, false);

	Ipc::Inst().AsyncInvoke("Checkpoint");

	return m_score_component->OpenFile(file_data);
}

QString ApplicationSut::TransfetString(const QString& str)
{
	return "return"+str;
}

//-------------------------------------------------------
void Ipc::InitSpecificClient(const QStringList& args)
{
	InitClient<ApplicationSut>(args);
}

void Ipc::SutRegister(QObject* sut_object)
{
	mass_return_if_equal(bool(ApplicationSut::m_int), false);
	if( ApplicationUnderTest* score_component = dynamic_cast<ApplicationUnderTest*>(sut_object) )
		ApplicationSut::m_int->Register(score_component);
}