#pragma once

#include <QtNetwork/QTcpServer>
#include "IpcTcpTransportQt.h"

class IpcServerTcpTransport: public IpcTcpTransportQt
{
public:
	IpcServerTcpTransport(uint16_t listen_port);
	~IpcServerTcpTransport();

	QTcpServer* Server();

	bool ListenRestart();

private:

	void NewConnection();

private:

	const uint16_t m_listen_port;
	QTcpServer	m_server;
};

