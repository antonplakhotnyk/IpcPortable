#include "ApplicationSut.h"
#include "IpcDataStreamSerializersQt.h"
#include "IpcQt_Global.h"


std::shared_ptr<ApplicationSut> ApplicationSut::m_int;

ApplicationSut::ApplicationSut(const IpcClientTcpTransport::Addr& connect_to_address)
	:m_testability(connect_to_address)
{

	{
		IpcClient().AddHandler("AutotestServerReady", std::function<void()>([this]()
		{
			m_server_ready_request_pending = true;
			AutotestServerReady();
		}), {}, ThreadCallerQt::GetCurrentThreadId(), this);
	}

	m_testability.Start();
}

void ApplicationSut::AutotestServerReady()
{
	if( m_server_ready_request_pending )
	{
		m_server_ready_request_pending = false;
		IpcClient().AsyncInvoke("SutReady");
	}
}

ApplicationSut::~ApplicationSut()
{
	Unregister(m_score_component);
}

void ApplicationSut::Init(const IpcClientTcpTransport::Addr& connect_to_address)
{
	if( !m_int )
		m_int.reset(new ApplicationSut(connect_to_address));
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

	MAssIpcCall& ipc = IpcClient();

	ipc.AddHandler("OpenFile", std::function<bool(QByteArray)>(std::bind(&ApplicationSut::OpenFile, this, std::placeholders::_1)), {}, ThreadCallerQt::GetCurrentThreadId(), score_component);
	ipc.AddHandler("TransfetString", std::function<QString(QString)>(std::bind(&ApplicationSut::TransfetString, this, std::placeholders::_1)), {}, ThreadCallerQt::GetCurrentThreadId(), score_component);

	AutotestServerReady();
}

void ApplicationSut::Unregister(QObject* obj)
{
	if( bool(m_score_component) && (m_score_component == obj) )
	{
		IpcClient().ClearHandlersWithTag(obj);
		m_score_component.clear();
	}
}

bool ApplicationSut::OpenFile(const QByteArray& file_data)
{
	mass_return_x_if_equal(bool(m_score_component), false, false);

	IpcClient().AsyncInvoke("Checkpoint");

	return m_score_component->OpenFile(file_data);
}

QString ApplicationSut::TransfetString(const QString& str)
{
	return "return"+str;
}
