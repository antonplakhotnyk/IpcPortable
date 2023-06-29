#pragma once

#include "SutHandlerThread.h"
#include "IpcQt_TransthreadCaller.h"
#include "IpcQt_TransportTcpServer.h"
#include "IpcQt_Net.h"


class AutotestServer: public IpcQt_TransportTcpServer
{
private:

	struct SutConnection
	{
		SutConnection(MAssIpcCall& ipc_connection, std::shared_ptr<IpcQt_TransporTcp> transport)
			: ipc_net(ipc_connection, transport)
		{
			this->ipc_net.ipc_call.SetErrorHandler(&SutConnection::IpcError);
		}

		IpcQt_Net		ipc_net;

	private:
		static void IpcError(MAssIpcCall::ErrorType et, std::string message);
	};


public:

	class Client
	{
	public:

		enum struct SutIndexId:size_t
		{
			on_connected_sut = 16,
			max_count = on_connected_sut+1,
		};


		struct Handlers
		{
			std::function<SutIndexId()> OnConnectedSut;
			std::function<void()> OnDisconnectedSut;
		};

		Client(const Handlers& handlers)
			: m_handlers(handlers)
		{
		}

		const MAssIpcCall& IpcCall(SutIndexId sut_index_id = {}) const
		{
			if( sut_index_id >=SutIndexId::max_count )
				return m_disconnected_stub;
			if( !bool(m_client_connections[size_t(sut_index_id)]) )
				return m_disconnected_stub;

			return m_client_connections[size_t(sut_index_id)]->ipc_net.ipc_call;
		};

	private:

		MAssIpcCall		m_disconnected_stub{std::weak_ptr<MAssIpc_Transthread>{}};

	protected:

		std::shared_ptr<SutConnection> m_client_connections[size_t(SutIndexId::max_count)];
		Handlers		m_handlers;
	};


	struct Params
	{
		TAutotestCreate autotest_container;
		uint16_t server_port = 2233;
	};

	AutotestServer(const Params& params);

	std::shared_ptr<Client> CreateClient(const Client::Handlers& handlers);

private:

	class ClientPrivate: public Client
	{
	public:
		template<class... Args>
		ClientPrivate(Args&&... args):Client(args...){};

		const Handlers& GetHandlers() const {return m_handlers;}
		void SetConnection(SutIndexId sut_id, std::shared_ptr<SutConnection> connection)
		{
			mass_return_if_equal(sut_id<SutIndexId::max_count, false);
			m_client_connections[size_t(sut_id)] = connection;
		}
	};


	void OnConnected(std::weak_ptr<IpcQt_TransporTcp> transport) override;
	void OnDisconnected(std::weak_ptr<IpcQt_TransporTcp> transport) override;

private:

	std::set<std::shared_ptr<SutConnection>>	m_connections;

	decltype(Params::autotest_container)		m_autotest_container;
	std::unique_ptr<SutHandlerThread>			m_sut_handler;


	std::vector<std::weak_ptr<ClientPrivate>>	m_clients;
};
