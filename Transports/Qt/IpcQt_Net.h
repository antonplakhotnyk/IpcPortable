#pragma once

#include "MAssIpcCall.h"
#include "IpcQt_TransporTcp.h"
#include "IpcQt_TransthreadTransportCopy.h"


class IpcQt_Net: public QObject
{
public:

	IpcQt_Net(MAssIpcCall& ipc_connection, std::shared_ptr<IpcQt_TransporTcp> transport)
		: m_ipc_transport_thread{MAssIpc_TransthreadTarget::CurrentThread()}
		, m_thread_transport{std::make_shared<IpcQt_TransthreadCaller>()}
		, m_transport_from_thread{std::make_shared<IpcQt_TransthreadTransportCopy>(m_ipc_transport_thread, transport, m_thread_transport)}
		, ipc_call(m_transport_from_thread, m_thread_transport)
	{
		QObject::connect(transport.get(), &IpcQt_TransporTcp::HandlerProcessTransport, this, [this]()
		{
			ipc_call.ProcessTransport();
		});
		this->ipc_call.AddAllHandlers(ipc_connection);
		// ipc_connection = ipc_call;
	}

private:

	MAssIpc_TransthreadTarget::Id					m_ipc_transport_thread;
	std::shared_ptr<IpcQt_TransthreadCaller>		m_thread_transport;
	std::shared_ptr<IpcQt_TransthreadTransportCopy>	m_transport_from_thread;
public:

	MAssIpcCall ipc_call;
};
