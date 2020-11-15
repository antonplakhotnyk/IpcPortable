#pragma once

#include "IpcCall.h"
#include <QtCore/QDataStream>
#include <QtNetwork/QHostAddress>
#include "IpcTcpTransportQt.h"
#include "EventgateW.h"


class IpcClientTcpTransport: public IpcTcpTransportQt
{
public:

	IpcClientTcpTransport(void);
	~IpcClientTcpTransport();

	void Init(const Handlers& handlers, QHostAddress address, uint16_t target_port);

	void WaitConnection();

private:
};


