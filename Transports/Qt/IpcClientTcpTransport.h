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

	struct Addr
	{
		QString host_name;
		uint16_t target_port;
	};

	void Init(const Addr& addr);

	void WaitConnection();

private:
};


