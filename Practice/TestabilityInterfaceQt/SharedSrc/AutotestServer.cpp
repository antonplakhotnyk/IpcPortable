#include "AutotestServer.h"
#include "MAssIpc_Macros.h"

AutotestServer::AutotestServer(const Params& params)
	:IpcQt_TransportTcpServer(params.server_port)
	, m_autotest_container(params.autotest_container)
{
	this->ListenRestart();
}

void AutotestServer::SutConnection::IpcError(MAssIpcCall::ErrorType et, std::string message)
{
	mass_assert_msg(std::string(std::string(MAssIpcCall::ErrorTypeToStr(et)) + " " + message).c_str());
}

void AutotestServer::OnConnected(std::weak_ptr<IpcQt_TransporTcp> transport_weak)
{
	auto transport = transport_weak.lock();

	MAssIpcCall ipc_call_initial({});
	std::shared_ptr<SutConnection> connection{std::make_shared<SutConnection>(ipc_call_initial, transport)};

	m_sut_handler = std::make_unique<SutHandlerThread>(connection->ipc_net.ipc_call, m_autotest_container);

	{
		auto clients{m_clients};
		for( std::weak_ptr<ClientPrivate> client_weak : clients )
			if( std::shared_ptr<ClientPrivate> client = client_weak.lock() )
			{
				client->SetConnection(Client::SutIndexId::on_connected_sut, connection);
				Client::SutIndexId sut_index_id = client->GetHandlers().OnConnectedSut();
				client->SetConnection(Client::SutIndexId::on_connected_sut, {});

				if( sut_index_id<Client::SutIndexId::on_connected_sut )
					client->SetConnection(sut_index_id, connection);
			}
	}
}

void AutotestServer::OnDisconnected(std::weak_ptr<IpcQt_TransporTcp> transport)
{
	m_sut_handler.reset();
	this->ListenRestart();

	{
		auto clients{m_clients};
		for( std::weak_ptr<ClientPrivate> client_weak : clients )
			if( std::shared_ptr<ClientPrivate> client = client_weak.lock() )
				client->GetHandlers().OnDisconnectedSut();
	}
}

std::shared_ptr<AutotestServer::Client> AutotestServer::CreateClient(const Client::Handlers& handlers)
{
	std::shared_ptr<AutotestServer::ClientPrivate> new_client{new AutotestServer::ClientPrivate(handlers)};
	m_clients.push_back(new_client);
	return new_client;
}

//-------------------------------------------------------
void AutotestServer::ClientPrivate::SetConnection(SutIndexId sut_id, std::shared_ptr<SutConnection> connection)
{
	mass_return_if_equal(sut_id<SutIndexId::max_count, false);
	m_client_connections[size_t(sut_id)] = connection;
}
