#include "IpcQt_TransportTcpServer.h"


IpcQt_TransportTcpServer::IpcQt_TransportTcpServer(uint16_t listen_port)
	:m_listen_port(listen_port)
{
	QObject::connect(&m_server, &QTcpServer::newConnection, this, &IpcQt_TransportTcpServer::NewConnection);
}

IpcQt_TransportTcpServer::~IpcQt_TransportTcpServer()
{
}

bool IpcQt_TransportTcpServer::ListenRestart()
{
	return m_server.listen(QHostAddress::Any, m_listen_port);
}

void IpcQt_TransportTcpServer::NewConnection()
{
	QTcpSocket* connection = m_server.nextPendingConnection();
	AssignConnection(connection);
	m_server.close();
}

QTcpServer* IpcQt_TransportTcpServer::Server()
{
	return &m_server;
}
