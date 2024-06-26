#pragma once

#include "SutHandlerThread.h"
#include "IpcQt_TransthreadCaller.h"
#include "IpcQt_Net.h"
#include "MAssIpcWaiter.h"
#include "TestabilityThreadQt.h"
#include "IpcQt_TransportTcpServer.h"
#include "TestabilityWaitReady.h"

class AutotestServer
{
public:

	struct Params
	{
		TAutotestCreate autotest_container;
		uint16_t server_port = 2233;
	};

	AutotestServer(const Params& params);
	~AutotestServer();

	std::shared_ptr<AutotestServer_Client> CreateClient();
 	void		Stop();
	size_t		GetConnectionsCount() const;

private:

	class ClientPrivate: public AutotestServer_Client
	{
	public:
		template<class... Args>
		ClientPrivate(Args&&... args):AutotestServer_Client(args...){};

		void SetOnConnectedSut(std::weak_ptr<SutConnection> connection)
		{
			std::unique_lock<std::recursive_mutex> lock(m_client_internals->m_connections_mutex);
			SetConnectionLocked(SutIndexId::on_connected_sut, connection);
		}

//		using AutotestServer_Client::RemoveConnectionLocked;
	};


	class ServerInternals: public IpcQt_TransportTcpServer
	{
	public:
		ServerInternals(const Params& params, const std::weak_ptr<AutotestClient_Internals>& client_internals)
			:IpcQt_TransportTcpServer(params.server_port)
			, m_autotest_container(params.autotest_container)
			, m_client_internals(client_internals)
		{
		}

		void AddClient(const std::shared_ptr<ClientPrivate>& new_client);

	private:

		decltype(Params::autotest_container)		m_autotest_container;
		std::unique_ptr<SutHandlerThread>			m_sut_handler;

		std::vector<std::weak_ptr<ClientPrivate>>	m_clients;
		std::recursive_mutex						m_clients_mutex;

		const std::weak_ptr<AutotestClient_Internals> m_client_internals;

	private:
		void OnConnected(std::weak_ptr<IpcQt_TransporTcp> transport) override;
		void OnDisconnected(std::weak_ptr<IpcQt_TransporTcp> transport) override;

		void AddNewConnection(const std::weak_ptr<IpcQt_TransporTcp>& transport, std::shared_ptr<AutotestClient_Internals::SutConnection> connection);
		void RemoveConnection(const std::weak_ptr<IpcQt_TransporTcp>& transport);

		const decltype(AutotestServer::ServerInternals::m_clients) RemoveExpiredClients();
	};

	void Background_Main(const Params& params);

private:

	TestabilityWaitReady						m_int_ready;
	IpcQt_TransthreadCaller						m_transthread_caller;
	std::weak_ptr<AutotestClient_Internals>		m_client_internals;
	std::weak_ptr<ServerInternals>				m_server_internals;
	std::shared_ptr<MAssIpcWaiter>              m_waiter;

	TestabilityThreadQt							m_background_thread;
};
