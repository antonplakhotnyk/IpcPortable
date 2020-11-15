#pragma once

#include "IpcCall.h"
#include "EventgateW.h"
#include "IpcServerTcpTransport.h"


class IpcServerNet
{
public:

	struct Handlers
	{
		EventgateW<void()> OnDisconnected;
		EventgateW<void()> OnConnected;
	};

	IpcServerNet(void);

	void Init(const Handlers& handlers, uint16_t listen_port);

	IpcCall& Call();

private:

	OPtr<IpcServerTcpTransport> m_transport;
	OPtr<SO<IpcCall> > m_ipc_call;
};

