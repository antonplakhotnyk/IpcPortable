#include "AutotestServer.h"
#include "MAssIpc_Macros.h"

AutotestServer::AutotestServer(const Params& params)
	:IpcQt_TransportTcpServer(params.server_port)
	, m_autotest_container(params.autotest_container)
{
	this->ListenRestart();
}

void AutotestServer::OnConnected(std::weak_ptr<IpcQt_TransporTcp> transport_weak)
{
	auto transport = transport_weak.lock();

	MAssIpcCall ipc_call_initial({});
	std::shared_ptr<AutotestServer_Client::SutConnection> connection{std::make_shared<AutotestServer_Client::SutConnection>(ipc_call_initial, transport, m_server_internals)};

	if( m_autotest_container )
		m_sut_handler = std::make_unique<SutHandlerThread>(connection, m_autotest_container);

	{
		auto clients{m_clients};
		for( std::weak_ptr<ClientPrivate> client_weak : clients )
			if( std::shared_ptr<ClientPrivate> client = client_weak.lock() )
			{
				client->SetConnection(AutotestServer_Client::SutIndexId::on_connected_sut, connection);
				AutotestServer_Client::SutIndexId sut_index_id = client->GetHandlers().OnConnectedSut();
				client->SetConnection(AutotestServer_Client::SutIndexId::on_connected_sut, {});

				if( sut_index_id<AutotestServer_Client::SutIndexId::on_connected_sut )
					client->SetConnection(sut_index_id, connection);
			}
	}

	m_server_internals->m_connection_signaling->SignalChangeState(MAssIpcWaiter::ConnectionEvent::State::on_connected);
}

void AutotestServer::OnDisconnected(std::weak_ptr<IpcQt_TransporTcp> transport)
{
	m_server_internals->m_connection_signaling->SignalChangeState(MAssIpcWaiter::ConnectionEvent::State::on_disconnected);

	m_sut_handler.reset();
//	this->ListenRestart();

	{
		auto clients{m_clients};
		for( std::weak_ptr<ClientPrivate> client_weak : clients )
			if( std::shared_ptr<ClientPrivate> client = client_weak.lock() )
				if( auto OnDisconnectedSut = client->GetHandlers().OnDisconnectedSut )
					OnDisconnectedSut();
	}
}

std::shared_ptr<AutotestServer_Client> AutotestServer::CreateClient(const AutotestServer_Client::Handlers& handlers)
{
	std::shared_ptr<AutotestServer::ClientPrivate> new_client{new AutotestServer::ClientPrivate(handlers, m_server_internals)};
	m_clients.push_back(new_client);
	return new_client;
}

void		AutotestServer::Stop()
{
	Server()->close();
	m_server_internals->m_waiter->CancelAllWaits();
}

//-------------------------------------------------------
void AutotestServer::ClientPrivate::SetConnection(SutIndexId sut_id, std::shared_ptr<AutotestServer_Client::SutConnection> connection)
{
	mass_return_if_equal(sut_id<SutIndexId::max_count, false);
	m_client_connections[size_t(sut_id)] = connection;
}
