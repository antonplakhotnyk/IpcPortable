#pragma once

#include <QtNetwork/QTcpServer>
#include "IpcTcpTransportQt.h"

class IpcServerTcpTransport: public IpcTcpTransportQt
{
public:

	void Init(uint16_t listen_port);

	IpcServerTcpTransport(void);
	~IpcServerTcpTransport();


	QTcpServer* Server();

private:

	void NewConnection();

private:

	QTcpServer	m_server;
};

