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
	std::shared_ptr<IpcQt_TransporTcp> transport{std::make_shared<IpcQt_TransporTcp>()};
	m_connection_transports.insert(transport);

	QObject::connect(transport.get(), &IpcQt_TransporTcp::HandlerOnConnected, this, &IpcQt_TransportTcpServer::OnConnected, Qt::QueuedConnection);
	QObject::connect(transport.get(), &IpcQt_TransporTcp::HandlerOnDisconnected, this, &IpcQt_TransportTcpServer::OnDisconnected, Qt::QueuedConnection);

	transport->AssignConnection(transport, connection);
//	m_server.close();
}

QTcpServer* IpcQt_TransportTcpServer::Server()
{
	return &m_server;
}
