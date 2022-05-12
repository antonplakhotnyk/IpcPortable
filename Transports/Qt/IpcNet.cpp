#include "IpcNet.h"

IpcNet::IpcNet(std::shared_ptr<IpcTcpTransportQt> transport)
{
	MAssIpcThreadTransportTarget::Id ipc_transport_thread = ThreadCallerQt::GetCurrentThreadId();
	m_thread_transport = std::make_shared<IpcThreadTransportQt>();
	m_transport_from_thread = std::make_shared<CallTransportFromQThread>(ipc_transport_thread, transport, m_thread_transport);
	m_ipc_call = std::make_unique<MAssIpcCall>(m_transport_from_thread, m_thread_transport);

	QObject::connect(transport.get(), &IpcTcpTransportQt::HandlerProcessTransport, this, std::bind(&MAssIpcCall::ProcessTransport, m_ipc_call.get()));
}

MAssIpcCall& IpcNet::Call() const
{
	return *m_ipc_call;
}
