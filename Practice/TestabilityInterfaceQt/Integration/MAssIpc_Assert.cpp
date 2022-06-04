#include "MAssIpcAssert.h"
#include <QtCore/qDebug>

void MAssIpcAssert::Raise(const char* file, int line, const char* func, const wchar_t* msg)
{
	qDebug()<<file<<"("<<line<<"): "<<msg;
}
