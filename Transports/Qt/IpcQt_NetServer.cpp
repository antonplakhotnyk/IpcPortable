#include "IpcQt_NetServer.h"


IpcQt_NetServer::IpcQt_NetServer(uint16_t listen_port)
	:m_transport(std::make_shared<IpcQt_TransportTcpServer>(listen_port))
	, m_ipc_connection({})
	, m_ipc_net(m_ipc_connection, m_transport)
{
}

void IpcQt_NetServer::Init()
{
	m_transport->ListenRestart();
}

MAssIpcCall& IpcQt_NetServer::Call()
{
	return m_ipc_connection;
}

IpcQt_TransportTcpServer& IpcQt_NetServer::GetIpcServerTcpTransport()
{
	return *m_transport.get();
}

