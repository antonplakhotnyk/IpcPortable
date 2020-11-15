#include "stdafx.h"
#include "IpcServerNet.h"


IpcServerNet::IpcServerNet(void)
{
	m_transport = new IpcServerTcpTransport;
	m_ipc_call = new MAssIpcCall(m_transport);
}

void IpcServerNet::Init(uint16_t listen_port)
{
	{
		IpcServerTcpTransport::Handlers transport_handlers;
		transport_handlers.OnDisconnected = handlers.OnDisconnected;
		transport_handlers.OnConnected = handlers.OnConnected;
		transport_handlers.ProcessTransport.BindE(&IpcCall::ProcessTransport, m_ipc_call);
		m_transport->Init(transport_handlers, listen_port);
	}
}

MAssIpcCall& IpcServerNet::Call()
{
	return *m_ipc_call;
}
