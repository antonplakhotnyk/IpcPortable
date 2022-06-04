#pragma once

#include "ApplicationUnderTest.h"
#include "TestabilityClientQt.h"


class ApplicationSut: public QObject
{
public:
	ApplicationSut(const IpcClientTcpTransport::Addr& connect_to_address);
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

	friend class Ipc;

	static std::shared_ptr<ApplicationSut> m_int;
};

