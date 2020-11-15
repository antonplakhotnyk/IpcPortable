#include "StdAfx.h"
#include "IpcServerTcpTransport.h"


IpcServerTcpTransport::IpcServerTcpTransport(void)
{
	QObject::connect(&m_server, &QTcpServer::newConnection, this, &IpcServerTcpTransport::NewConnection);
}

IpcServerTcpTransport::~IpcServerTcpTransport()
{
}

void IpcServerTcpTransport::Init(const Handlers& handlers, uint16_t listen_port)
{
	IpcTcpTransportQt::Init(handlers);
	m_server.listen(QHostAddress::Any, listen_port);
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
