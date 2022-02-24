#pragma once

#include "MAssIpcCallDataStream.h"
#include <QtCore/QString>

MAssIpcCallDataStream& operator<<(MAssIpcCallDataStream& stream, const QString& v);
MAssIpcCallDataStream& operator>>(MAssIpcCallDataStream& stream, QString& v);

MASS_IPC_TYPE_SIGNATURE(QString);

//-------------------------------------------------------



