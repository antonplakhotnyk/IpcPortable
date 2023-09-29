#include "AutotestServer.h"
#include "MAssIpc_Macros.h"

AutotestServer::AutotestServer(const Params& params)
	:m_background_thread(nullptr, [this, params](){Background_Main(params);})
{
	m_background_thread.start();
	m_int_ready.Wait();
}

AutotestServer::~AutotestServer()
{
}

void AutotestServer::Background_Main(const Params& params)
{
	QEventLoop event_loop;
	std::shared_ptr<AutotestClient_Internals> client_internals = std::make_shared<AutotestClient_Internals>();
	std::shared_ptr<ServerInternals>	transport_tcp_server = std::make_shared<ServerInternals>(params, client_internals);

	m_client_internals = client_internals;
	m_server_internals = transport_tcp_server;
	m_int_ready.SetReady();

	transport_tcp_server->ListenRestart();

	event_loop.exec();

	transport_tcp_server->Server()->close();
	client_internals->m_waiter->CancelAllWaits();
}

const decltype(AutotestServer::ServerInternals::m_clients) AutotestServer::ServerInternals::RemoveExpiredClients()
{
	std::unique_lock<std::recursive_mutex> lock(m_clients_mutex);

	auto new_end = std::remove_if(m_clients.begin(), m_clients.end(),[](const std::weak_ptr<ClientPrivate>& client){return client.expired();});
	m_clients.erase(new_end, m_clients.end());

	return m_clients;
}

void AutotestServer::ServerInternals::AddClient(const std::weak_ptr<ClientPrivate>& new_client)
{
	std::unique_lock<std::recursive_mutex> lock(m_clients_mutex);
	m_clients.push_back(new_client);
}

void AutotestServer::ServerInternals::OnConnected(std::weak_ptr<IpcQt_TransporTcp> transport_weak)
{
	auto transport = transport_weak.lock();
	auto client_internals = m_client_internals.lock();

	MAssIpcCall ipc_call_initial({});
	std::shared_ptr<AutotestServer_Client::SutConnection> connection{std::make_shared<AutotestServer_Client::SutConnection>(ipc_call_initial, transport, m_client_internals)};

	if( m_autotest_container )
		m_sut_handler = std::make_unique<SutHandlerThread>(connection, m_autotest_container);

	const decltype(m_clients) clients = RemoveExpiredClients();
	for( auto client:clients )
		client.lock()->AddNewConnection(transport_weak, connection);

	client_internals->m_connection_signaling->SignalChangeState(MAssIpcWaiter::ConnectionEvent::State::on_connected);
}

void AutotestServer::ServerInternals::OnDisconnected(std::weak_ptr<IpcQt_TransporTcp> transport)
{
	auto client_internals = m_client_internals.lock();
	client_internals->m_connection_signaling->SignalChangeState(MAssIpcWaiter::ConnectionEvent::State::on_disconnected);

	const decltype(m_clients) clients = RemoveExpiredClients();
	for( auto client:clients )
		client.lock()->RemoveConnection(transport);


	m_sut_handler.reset();
//	this->ListenRestart();
}

std::shared_ptr<AutotestServer_Client> AutotestServer::CreateClient(/*const AutotestServer_Client::Handlers& handlers*/)
{
	auto server_internals = m_server_internals.lock();
	mass_return_x_if_equal(bool(server_internals), false, {});

	std::shared_ptr<AutotestServer::ClientPrivate> new_client{new AutotestServer::ClientPrivate(/*handlers,*/ m_client_internals)};
	server_internals->AddClient(new_client);
	return new_client;
}

void		AutotestServer::Stop()
{
	m_background_thread.Stop();
}

//-------------------------------------------------------
void AutotestServer::ClientPrivate::SetConnectionLocked(SutIndexId sut_id, std::shared_ptr<SutConnection> connection)
{
	mass_return_if_equal(sut_id<SutIndexId::max_count, false);
	m_client_connections[size_t(sut_id)] = connection;
}
