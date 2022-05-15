#pragma once

class ApplicationUnderTest: public QObject
{
public:

	ApplicationUnderTest();
	~ApplicationUnderTest();

	bool OpenFile(const QByteArray& file_data);


};