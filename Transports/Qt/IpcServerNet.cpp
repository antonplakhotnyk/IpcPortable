#include "IpcServerNet.h"


IpcServerNet::IpcServerNet(uint16_t listen_port)
	:m_transport(std::make_shared<IpcServerTcpTransport>(listen_port))
	, m_ipc_net(m_transport)
{
}

void IpcServerNet::Init()
{
	m_transport->ListenRestart();
}

MAssIpcCall& IpcServerNet::Call() const
{
	return m_ipc_net.Call();
}

IpcServerTcpTransport& IpcServerNet::GetIpcServerTcpTransport()
{
	return *m_transport.get();
}

