#pragma once

#include "MAssIpcCall.h"
#include "IpcServerTcpTransport.h"
#include "IpcThreadTransportQt.h"


class IpcServerNet: public QObject
{
public:

	IpcServerNet(uint16_t listen_port);

	void Init();

	MAssIpcCall& Call();

	IpcServerTcpTransport& GetIpcServerTcpTransport();

private:

	std::shared_ptr<IpcServerTcpTransport> m_transport;
	std::shared_ptr<IpcThreadTransportQt> m_thread_transport;
	std::unique_ptr<MAssIpcCall> m_ipc_call;
};

