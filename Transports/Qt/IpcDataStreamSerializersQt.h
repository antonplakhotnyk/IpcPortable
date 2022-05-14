#pragma once

#include "MAssIpcCallDataStream.h"
#include <QtCore/QString>
#include <QtCore/QByteArray>

MAssIpcCallDataStream& operator<<(MAssIpcCallDataStream& stream, const QString& v);
MAssIpcCallDataStream& operator>>(MAssIpcCallDataStream& stream, QString& v);

MAssIpcCallDataStream& operator<<(MAssIpcCallDataStream& stream, const QByteArray& v);
MAssIpcCallDataStream& operator>>(MAssIpcCallDataStream& stream, QByteArray& v);

MASS_IPC_TYPE_SIGNATURE(QString);
MASS_IPC_TYPE_SIGNATURE(QByteArray);

//-------------------------------------------------------



