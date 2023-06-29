#include "IpcQt_Net.h"

IpcQt_Net::IpcQt_Net(MAssIpcCall& ipc_connection, std::shared_ptr<IpcQt_TransporTcp> transport)
	: m_ipc_transport_thread{IpcQt_TransthreadCaller::GetCurrentThreadId()}
	, m_thread_transport{std::make_shared<IpcQt_Transthread>()}
	, m_transport_from_thread{std::make_shared<IpcQt_TransthreadTransportCopy>(m_ipc_transport_thread, transport, m_thread_transport)}
	, ipc_call(m_transport_from_thread, m_thread_transport)
{
	QObject::connect(transport.get(), &IpcQt_TransporTcp::HandlerProcessTransport, this, [this](){ipc_call.ProcessTransport();});
	this->ipc_call.AddAllHandlers(ipc_connection);
	// ipc_connection = ipc_call;
}
