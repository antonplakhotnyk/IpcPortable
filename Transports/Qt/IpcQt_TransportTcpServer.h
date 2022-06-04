#pragma once

#include <QtNetwork/QTcpServer>
#include "IpcQt_TransporTcp.h"

class IpcQt_TransportTcpServer: public IpcQt_TransporTcp
{
public:
	IpcQt_TransportTcpServer(uint16_t listen_port);
	~IpcQt_TransportTcpServer();

	QTcpServer* Server();

	bool ListenRestart();

private:

	void NewConnection();

private:

	const uint16_t m_listen_port;
	QTcpServer	m_server;
};

