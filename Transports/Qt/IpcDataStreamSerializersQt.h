#pragma once

#include "IpcCallDataStream.h"
#include <QString>

IpcCallDataStream& operator<<(IpcCallDataStream& stream, const QString& v);
IpcCallDataStream& operator>>(IpcCallDataStream& stream, QString& v);

AV_IPC_TYPE_SIGNATURE(QString);

//-------------------------------------------------------



