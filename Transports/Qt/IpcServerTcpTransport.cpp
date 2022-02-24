#include "IpcServerTcpTransport.h"


IpcServerTcpTransport::IpcServerTcpTransport(uint16_t listen_port)
	:m_listen_port(listen_port)
{
	QObject::connect(&m_server, &QTcpServer::newConnection, this, &IpcServerTcpTransport::NewConnection);
}

IpcServerTcpTransport::~IpcServerTcpTransport()
{
}

bool IpcServerTcpTransport::ListenRestart()
{
	return m_server.listen(QHostAddress::Any, m_listen_port);
}

void IpcServerTcpTransport::NewConnection()
{
	QTcpSocket* connection = m_server.nextPendingConnection();
	AssignConnection(connection);
	m_server.close();
}

QTcpServer* IpcServerTcpTransport::Server()
{
	return &m_server;
}
