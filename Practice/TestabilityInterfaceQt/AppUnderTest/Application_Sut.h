#pragma once

#include "ApplicationUnderTest.h"
#include "TestabilityAppSutBaseQt.h"


class Application_Sut: public TestabilityAppSutBaseQt, public Sut
{
public:

	Application_Sut(const TestabilityGlobalQt::Addr& connect_to_address, std::weak_ptr<EventHandlerMap>* sut_event_map);
	~Application_Sut();

private:


	void Register(ApplicationUnderTest* application);
	void Unregister(QObject* obj);

	bool OpenFile(const QByteArray& file_data);
	QString TransfetString(const QString& str);

private:

	QPointer<ApplicationUnderTest> m_application;
};

