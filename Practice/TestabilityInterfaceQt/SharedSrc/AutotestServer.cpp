#include "AutotestServer.h"

AutotestServer::AutotestServer(const Params& params)
	:m_transport_server(std::make_shared<IpcServerTcpTransport>(params.server_port))
	, m_ipc_connection({})
	, m_ipc_net(m_ipc_connection, m_transport_server)
	, m_autotest_container(params.autotest_container)
{
	QObject::connect(m_transport_server.get(), &IpcTcpTransportQt::HandlerOnConnected, this, &AutotestServer::OnConnected, Qt::QueuedConnection);
	QObject::connect(m_transport_server.get(), &IpcTcpTransportQt::HandlerOnDisconnected, this, &AutotestServer::OnDisconnected, Qt::QueuedConnection);
	
	m_transport_server->ListenRestart();
}

void AutotestServer::OnConnected()
{
	m_sut_handler = std::make_unique<SutHandlerThread>(m_ipc_connection, m_autotest_container);
}

void AutotestServer::OnDisconnected()
{
	m_sut_handler.reset();
	m_transport_server->ListenRestart();
}

