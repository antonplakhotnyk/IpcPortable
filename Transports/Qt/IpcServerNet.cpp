#include "IpcServerNet.h"


IpcServerNet::IpcServerNet(uint16_t listen_port)
{
	m_transport = std::make_shared<IpcServerTcpTransport>(listen_port);
	m_thread_transport = std::make_shared<IpcThreadTransportQt>();
	m_ipc_call = std::make_unique<MAssIpcCall>(m_transport, m_thread_transport);

	QObject::connect(m_transport.get(), &IpcTcpTransportQt::HandlerProcessTransport, this, std::bind(&MAssIpcCall::ProcessTransport, m_ipc_call.get()) );
}

void IpcServerNet::Init()
{
	m_transport->ListenRestart();
}

MAssIpcCall& IpcServerNet::Call()
{
	return *m_ipc_call;
}

IpcServerTcpTransport& IpcServerNet::GetIpcServerTcpTransport()
{
	return *m_transport.get();
}

