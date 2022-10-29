#pragma once

#include "MAssIpcCall.h"
#include <QtCore/QDataStream>
#include <QtNetwork/QHostAddress>
#include "IpcQt_TransporTcp.h"
#include "IpcQt_Global.h"


class IpcQt_TransportTcpClient: public IpcQt_TransporTcp
{
public:

	IpcQt_TransportTcpClient(void);
	~IpcQt_TransportTcpClient();

	void StartConnection(const Ipc::Addr& addr);
	bool WaitConnection();

private:
};


