#include "ApplicationUnderTest.h"
#include "Application_Sut.h"

ApplicationUnderTest::ApplicationUnderTest()
{
	Sut::RegisterObject(this);
}

ApplicationUnderTest::~ApplicationUnderTest()
{
	Sut::UnregisterObject(this);
}

bool ApplicationUnderTest::OpenFile(const QByteArray& file_data)
{
	QString str = QString::fromUtf8(file_data);
	return (str!="return false");
}
