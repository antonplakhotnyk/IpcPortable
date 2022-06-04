#include "IpcNet.h"

IpcNet::IpcNet(MAssIpcCall& ipc_connection, std::shared_ptr<IpcTcpTransportQt> transport)
{
	MAssIpcThreadTransportTarget::Id ipc_transport_thread = ThreadCallerQt::GetCurrentThreadId();
	m_thread_transport = std::make_shared<IpcThreadTransportQt>();
	m_transport_from_thread = std::make_shared<CallTransportFromQThread>(ipc_transport_thread, transport, m_thread_transport);
	
	{
		MAssIpcCall new_ipc(m_transport_from_thread, m_thread_transport);
		new_ipc.AddAllHandlers(ipc_connection);
		ipc_connection = new_ipc;
	}

	QObject::connect(transport.get(), &IpcTcpTransportQt::HandlerProcessTransport, this, std::bind(&MAssIpcCall::ProcessTransport, &ipc_connection));
}
