#pragma once

#include "MAssIpcCall.h"
#include "IpcQt_TransporTcp.h"
#include "IpcQt_Transthread.h"
#include "IpcQt_TransthreadTransportCopy.h"


class IpcQt_Net: public QObject
{
public:

	IpcQt_Net(MAssIpcCall& ipc_connection, std::shared_ptr<IpcQt_TransporTcp> transport);

private:

	MAssIpc_TransthreadTarget::Id					m_ipc_transport_thread;
	std::shared_ptr<IpcQt_Transthread>		m_thread_transport;
	std::shared_ptr<IpcQt_TransthreadTransportCopy>	m_transport_from_thread;
public:

	MAssIpcCall ipc_call;
};
