#include "IpcQt_Net.h"

IpcQt_Net::IpcQt_Net(MAssIpcCall& ipc_connection, std::shared_ptr<IpcQt_TransporTcp> transport)
{
	MAssIpc_TransthreadTarget::Id ipc_transport_thread = IpcQt_TransthreadCaller::GetCurrentThreadId();
	m_thread_transport = std::make_shared<IpcQt_Transthread>();
	m_transport_from_thread = std::make_shared<IpcQt_TransthreadTransportCopy>(ipc_transport_thread, transport, m_thread_transport);
	
	{
		MAssIpcCall new_ipc(m_transport_from_thread, m_thread_transport);
		new_ipc.AddAllHandlers(ipc_connection);
		ipc_connection = new_ipc;
	}

	QObject::connect(transport.get(), &IpcQt_TransporTcp::HandlerProcessTransport, this, std::bind(&MAssIpcCall::ProcessTransport, &ipc_connection));
}
