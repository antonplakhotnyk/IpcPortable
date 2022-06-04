#pragma once

#include "MAssIpcCall.h"
#include "IpcQt_TransportTcpServer.h"
#include "IpcQt_Transthread.h"
#include "IpcQt_TransthreadTransportCopy.h"
#include "IpcQt_Net.h"

class IpcQt_NetServer: public QObject
{
public:

	IpcQt_NetServer(uint16_t listen_port);

	void Init();

	MAssIpcCall& Call();

	IpcQt_TransportTcpServer& GetIpcServerTcpTransport();

private:

	std::shared_ptr<IpcQt_TransportTcpServer> m_transport;
	
	MAssIpcCall		m_ipc_connection;
	IpcQt_Net			m_ipc_net;
};

