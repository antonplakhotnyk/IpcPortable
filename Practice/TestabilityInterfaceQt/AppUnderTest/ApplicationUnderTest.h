#pragma once

#include <QtCore/QObject>
#include <QtCore/QByteArray>

class ApplicationUnderTest: public QObject
{
public:

	ApplicationUnderTest();
	~ApplicationUnderTest();

	bool OpenFile(const QByteArray& file_data);


};