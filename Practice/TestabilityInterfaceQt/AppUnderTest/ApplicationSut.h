#pragma once

#include "ApplicationUnderTest.h"
#include "TestabilityClientQt.h"


class ApplicationSut: public QObject
{
public:

	ApplicationSut(const Ipc::Addr& connect_to_address, std::weak_ptr<EventHandlerMap>* sut_event_map);
	~ApplicationSut();

	static void ApplicationUnderTest_Register(ApplicationUnderTest* score_component);

private:


	void Register(ApplicationUnderTest* score_component);
	void Unregister(QObject* obj);

	bool OpenFile(const QByteArray& file_data);
	QString TransfetString(const QString& str);

private:

	void AutotestServerReady();
	bool m_server_ready_request_pending = false;

	QPointer<ApplicationUnderTest> m_score_component;
	TestabilityClientQt		m_testability;
	std::shared_ptr<EventHandlerMap> m_sut_event_map;

	friend class Ipc;
	using Internals = ApplicationSut;

public:
	static std::shared_ptr<ApplicationSut> s_sut_inst;
};

