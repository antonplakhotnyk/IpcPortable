#pragma once

#include "MAssIpcCall.h"
#include "IpcServerTcpTransport.h"
#include "IpcThreadTransportQt.h"
#include "CallTransportFromQThread.h"
#include "IpcNet.h"

class IpcServerNet: public QObject
{
public:

	IpcServerNet(uint16_t listen_port);

	void Init();

	MAssIpcCall& Call() const;

	IpcServerTcpTransport& GetIpcServerTcpTransport();

private:

	std::shared_ptr<IpcServerTcpTransport> m_transport;
	IpcNet	m_ipc_net;
};

