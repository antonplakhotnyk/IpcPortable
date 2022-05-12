#pragma once

#include "MAssIpcCall.h"
#include "IpcTcpTransportQt.h"
#include "IpcThreadTransportQt.h"
#include "CallTransportFromQThread.h"


class IpcNet: public QObject
{
public:

	IpcNet(std::shared_ptr<IpcTcpTransportQt> transport);

	MAssIpcCall& Call() const;

private:

	std::shared_ptr<IpcThreadTransportQt>		m_thread_transport;
	std::shared_ptr<CallTransportFromQThread>	m_transport_from_thread;
	std::unique_ptr<MAssIpcCall>				m_ipc_call;
};
