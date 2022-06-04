#pragma once

#include "MAssIpcCall.h"
#include <QtCore/QDataStream>
#include <QtNetwork/QHostAddress>
#include "IpcQt_TransporTcp.h"


class IpcQt_TransportTcpClient: public IpcQt_TransporTcp
{
public:

	IpcQt_TransportTcpClient(void);
	~IpcQt_TransportTcpClient();

	struct Addr
	{
		QString host_name;
		uint16_t target_port;
	};

	void StartConnection(const Addr& addr);
	bool WaitConnection();

private:
};


