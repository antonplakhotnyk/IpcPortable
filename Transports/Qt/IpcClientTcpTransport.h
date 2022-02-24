#pragma once

#include "MAssIpcCall.h"
#include <QtCore/QDataStream>
#include <QtNetwork/QHostAddress>
#include "IpcTcpTransportQt.h"


class IpcClientTcpTransport: public IpcTcpTransportQt
{
public:

	IpcClientTcpTransport(void);
	~IpcClientTcpTransport();

	void Init(QString address, uint16_t target_port);

	void WaitConnection();

private:
};


