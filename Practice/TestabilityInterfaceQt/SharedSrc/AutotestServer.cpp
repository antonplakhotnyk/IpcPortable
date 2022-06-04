#include "AutotestServer.h"
#include "MAssIpc_Macros.h"

AutotestServer::AutotestServer(const Params& params)
	:m_transport_server(std::make_shared<IpcQt_TransportTcpServer>(params.server_port))
	, m_ipc_connection({})
	, m_ipc_net(m_ipc_connection, m_transport_server)
	, m_autotest_container(params.autotest_container)
{
	m_ipc_connection.SetErrorHandler(&AutotestServer::IpcError);

	QObject::connect(m_transport_server.get(), &IpcQt_TransporTcp::HandlerOnConnected, this, &AutotestServer::OnConnected, Qt::QueuedConnection);
	QObject::connect(m_transport_server.get(), &IpcQt_TransporTcp::HandlerOnDisconnected, this, &AutotestServer::OnDisconnected, Qt::QueuedConnection);
	
	m_transport_server->ListenRestart();
}

void AutotestServer::IpcError(MAssIpcCall::ErrorType et, std::string message)
{
	mass_assert_msg(std::string(std::string(MAssIpcCall::ErrorTypeToStr(et)) + " " + message).c_str());
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

