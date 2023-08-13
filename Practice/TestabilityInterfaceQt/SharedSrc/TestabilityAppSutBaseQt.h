#pragma once

#include "TestabilityClientQt.h"


class TestabilityAppSutBaseQt: public QObject
{
public:

	TestabilityAppSutBaseQt(const TestabilityGlobalQt::Addr& connect_to_address, std::weak_ptr<EventHandlerMap>* sut_event_map);
	virtual ~TestabilityAppSutBaseQt();

protected:
	void SetApplicationReady(bool sut_ready);

private:

	void AutotestServerReady();

	bool m_sut_ready=false;
	bool m_server_ready_request_pending = false;

	TestabilityClientQt					m_testability;
	std::shared_ptr<EventHandlerMap>	m_sut_event_map;
};

