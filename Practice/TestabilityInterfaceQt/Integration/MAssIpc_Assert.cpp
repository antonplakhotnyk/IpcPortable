#include "MAssIpc_Assert.h"
#include <QtCore/qDebug>

void MAssIpc_Assert::Raise(const char* file, int line, const char* func, const wchar_t* msg)
{
	qDebug()<<file<<"("<<line<<"): "<<std::wstring(msg);
}
void MAssIpc_Assert::Raise(const char* file, int line, const char* func, const char* msg)
{
	qDebug()<<file<<"("<<line<<"): "<<msg;
}

void MAssIpc_Assert::Raise(const char* file, int line, const char* func, const MAssIpc_AutotestMsg& msg)
{
	qDebug()<<file<<"("<<line<<"): "<<"FAILED Autotest";
}
