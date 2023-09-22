#include "IpcQt_TransportTcpServer.h"
#include "MAssIpc_Macros.h"


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
	QObject::connect(transport.get(), &IpcQt_TransporTcp::HandlerOnDisconnected, this, &IpcQt_TransportTcpServer::ClearConnection, Qt::QueuedConnection);

	transport->AssignConnection(transport, connection);
//	m_server.close();
}

void IpcQt_TransportTcpServer::ClearConnection(std::weak_ptr<IpcQt_TransporTcp> transport)
{
 	auto connection_transport = transport.lock();
	mass_return_if_equal(bool(connection_transport), false);
	m_connection_transports.erase(connection_transport);
}

QTcpServer* IpcQt_TransportTcpServer::Server()
{
	return &m_server;
}
