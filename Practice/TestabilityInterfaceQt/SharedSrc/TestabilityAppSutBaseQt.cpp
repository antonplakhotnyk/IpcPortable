#include "TestabilityAppSutBaseQt.h"
#include "IpcQt_Serializers.h"
#include "TestabilityGlobalQt.h"


TestabilityAppSutBaseQt::TestabilityAppSutBaseQt(const TestabilityGlobalQt::Addr& connect_to_address, std::weak_ptr<EventHandlerMap>* sut_event_map)
	:m_testability(TestabilityGlobalQt::Ipc(), connect_to_address)
	, m_sut_event_map((*sut_event_map = std::make_shared<EventHandlerMap>()).lock())
{
	TestabilityGlobalQt::Ipc().AddHandler("AutotestServerReady", std::function<void()>([this]()
	{
		m_server_ready_request_pending = true;
		AutotestServerReady();
	}), this);

	m_testability.Start();
}

TestabilityAppSutBaseQt::~TestabilityAppSutBaseQt()
{
	TestabilityGlobalQt::Ipc().ClearHandlersWithTag(this);
}

void TestabilityAppSutBaseQt::AutotestServerReady()
{
	if( m_sut_ready && m_server_ready_request_pending )
	{
		m_server_ready_request_pending = false;
		TestabilityGlobalQt::Ipc().AsyncInvoke("SutReady");
	}
}

void TestabilityAppSutBaseQt::SetApplicationReady(bool sut_ready)
{
	m_sut_ready = sut_ready;
	AutotestServerReady();
}
