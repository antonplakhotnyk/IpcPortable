#pragma once

#include "MAssIpcCall.h"
#include "IpcTcpTransportQt.h"
#include "IpcThreadTransportQt.h"
#include "CallTransportFromQThread.h"


class IpcNet: public QObject
{
public:

	IpcNet(MAssIpcCall& ipc_connection, std::shared_ptr<IpcTcpTransportQt> transport);

private:

	std::shared_ptr<IpcThreadTransportQt>		m_thread_transport;
	std::shared_ptr<CallTransportFromQThread>	m_transport_from_thread;
};
