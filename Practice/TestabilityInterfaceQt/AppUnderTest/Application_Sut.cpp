#include "Application_Sut.h"
#include "IpcQt_Serializers.h"
#include "TestabilityGlobalQt.h"


Application_Sut::Application_Sut(const TestabilityGlobalQt::Addr& connect_to_address, std::weak_ptr<EventHandlerMap>* sut_event_map)
	:TestabilityAppSutBaseQt(connect_to_address, sut_event_map)
{
	Sut::AddHandler_RegisterObject<ApplicationUnderTest>([this](ApplicationUnderTest* application){Register(application);}, this);
	Sut::AddHandler_UnregisterObject<ApplicationUnderTest>([this](ApplicationUnderTest* application){Unregister(application);}, this);
}

Application_Sut::~Application_Sut()
{
	TestabilityGlobalQt::Ipc().ClearHandlersWithTag(this);
	Sut::ClearHandlersWithTag(this);
}

void Application_Sut::Register(ApplicationUnderTest* application)
{
	Unregister(m_application);
	m_application = application;

	QObject::connect(application, &QObject::destroyed, this, &Application_Sut::Unregister);

	MAssIpcCall& ipc = TestabilityGlobalQt::Ipc();

	ipc.AddHandler("OpenFile", [this](QByteArray file_data)->bool{return OpenFile(file_data);}, this);
	ipc.AddHandler("TransfetString", [this](QString str)->QString{return TransfetString(str);}, this);
	ipc.AddHandler("GetType", []()->std::string
	{
		return "Application_Sut";
	});
	

	SetApplicationReady(true);
}

void Application_Sut::Unregister(QObject* obj)
{
	if( bool(m_application) && (m_application == obj) )
	{
		SetApplicationReady(false);
		TestabilityGlobalQt::Ipc().ClearHandlersWithTag(obj);
		m_application.clear();
	}
}

bool Application_Sut::OpenFile(const QByteArray& file_data)
{
	return_x_if_equal(bool(m_application), false, false);

	TestabilityGlobalQt::Ipc().AsyncInvoke("Checkpoint");

	return m_application->OpenFile(file_data);
}

QString Application_Sut::TransfetString(const QString& str)
{
	return "return"+str;
}
