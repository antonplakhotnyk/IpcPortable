#include "AutotestServer.h"

AutotestServer::AutotestServer(const Params& params)
	:m_ipc_server(params.server_port)
	, m_autotest_container(params.autotest_container)
{
	QObject::connect(&m_ipc_server.GetIpcServerTcpTransport(), &IpcTcpTransportQt::HandlerOnConnected, this, &AutotestServer::OnConnected, Qt::QueuedConnection);
	QObject::connect(&m_ipc_server.GetIpcServerTcpTransport(), &IpcTcpTransportQt::HandlerOnDisconnected, this, &AutotestServer::OnDisconnected, Qt::QueuedConnection);
	m_ipc_server.Init();
}

void AutotestServer::OnConnected()
{
	m_sut_handler = std::make_unique<SutHandlerThread>(m_ipc_server.Call(), m_autotest_container);
}

void AutotestServer::OnDisconnected()
{
	m_sut_handler->terminate();
	m_ipc_server.GetIpcServerTcpTransport().ListenRestart();
}

