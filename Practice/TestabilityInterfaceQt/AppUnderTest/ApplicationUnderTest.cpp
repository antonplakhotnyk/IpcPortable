#include "ApplicationUnderTest.h"
#include "ApplicationSut.h"

ApplicationUnderTest::ApplicationUnderTest()
{
	ApplicationSut::ApplicationUnderTest_Register(this);
}

ApplicationUnderTest::~ApplicationUnderTest()
{
}

bool ApplicationUnderTest::OpenFile(const QByteArray& file_data)
{
	QString str = QString::fromUtf8(file_data);
	return (str!="return false");
}
