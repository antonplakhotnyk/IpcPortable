#pragma once

#include "MAssIpc_DataStream.h"
#include <QtCore/QString>
#include <QtCore/QByteArray>

MAssIpc_DataStream& operator<<(MAssIpc_DataStream& stream, const QString& v);
MAssIpc_DataStream& operator>>(MAssIpc_DataStream& stream, QString& v);

MAssIpc_DataStream& operator<<(MAssIpc_DataStream& stream, const QByteArray& v);
MAssIpc_DataStream& operator>>(MAssIpc_DataStream& stream, QByteArray& v);

MASS_IPC_TYPE_SIGNATURE(QString);
MASS_IPC_TYPE_SIGNATURE(QByteArray);

//-------------------------------------------------------



