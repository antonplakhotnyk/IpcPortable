#pragma once

#include "ApplicationUnderTest.h"
#include "TestabilityClientQt.h"


class ApplicationSut: public QObject
{
public:
	~ApplicationSut();

	static void	Init(const IpcClientTcpTransport::Addr& connect_to_address);
	static void ApplicationUnderTest_Register(ApplicationUnderTest* score_component);

private:

	ApplicationSut(const IpcClientTcpTransport::Addr& connect_to_address);

	void Register(ApplicationUnderTest* score_component);
	void Unregister(QObject* obj);

	bool OpenFile(const QByteArray& file_data);
	QString TransfetString(const QString& str);

private:

	QPointer<ApplicationUnderTest> m_score_component;
	TestabilityClientQt		m_testability;

	static std::shared_ptr<ApplicationSut> m_int;
};

