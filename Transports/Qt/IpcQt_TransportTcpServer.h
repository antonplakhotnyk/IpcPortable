#pragma once

#include <QtNetwork/QTcpServer>
#include "IpcQt_TransporTcp.h"
#include <set>

class IpcQt_TransportTcpServer: public QObject
{
public:
	IpcQt_TransportTcpServer(uint16_t listen_port);
	~IpcQt_TransportTcpServer();

	QTcpServer* Server();

	bool ListenRestart();

private:

	void NewConnection();
	void ClearConnection(std::weak_ptr<IpcQt_TransporTcp> transport);

	virtual void OnConnected(std::weak_ptr<IpcQt_TransporTcp> transport){};
	virtual void OnDisconnected(std::weak_ptr<IpcQt_TransporTcp> transport){};

private:

	const uint16_t m_listen_port;
	QTcpServer	m_server;
	std::set<std::shared_ptr<IpcQt_TransporTcp> > m_connection_transports;
};

