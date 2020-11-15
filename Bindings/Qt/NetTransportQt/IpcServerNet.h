#pragma once

#include "MAssIpcCall.h"
#include "IpcServerTcpTransport.h"


class IpcServerNet: public QObject
{
	Q_OBJECT
public:

	IpcServerNet(void);

	void Init(uint16_t listen_port);

	MAssIpcCall& Call();

signals:
	void OnDisconnected();
	void OnConnected();

private:

	std::unique_ptr<IpcServerTcpTransport> m_transport;
	std::shared_ptr<MAssIpcCall> m_ipc_call;
};

