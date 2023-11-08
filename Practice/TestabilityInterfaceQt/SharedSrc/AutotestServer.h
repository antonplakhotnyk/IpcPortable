#pragma once

#include "SutHandlerThread.h"
#include "IpcQt_TransthreadCaller.h"
#include "IpcQt_Net.h"
#include "MAssIpcWaiter.h"
#include "TestabilityThreadQt.h"
#include "IpcQt_TransportTcpServer.h"

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

private:

	class ClientPrivate: public AutotestServer_Client
	{
	public:
		template<class... Args>
		ClientPrivate(Args&&... args):AutotestServer_Client(args...){};

		using AutotestServer_Client::AddNewConnection;
		using AutotestServer_Client::RemoveConnection;
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

		void AddClient(const std::weak_ptr<ClientPrivate>& new_client);

	private:

		decltype(Params::autotest_container)		m_autotest_container;
		std::unique_ptr<SutHandlerThread>			m_sut_handler;

		std::vector<std::weak_ptr<ClientPrivate>>	m_clients;
		std::recursive_mutex						m_clients_mutex;

		const std::weak_ptr<AutotestClient_Internals> m_client_internals;

	private:
		void OnConnected(std::weak_ptr<IpcQt_TransporTcp> transport) override;
		void OnDisconnected(std::weak_ptr<IpcQt_TransporTcp> transport) override;

		const decltype(AutotestServer::ServerInternals::m_clients) RemoveExpiredClients();

	};

	void Background_Main(const Params& params);

private:

	TestabilityWaitReady						m_int_ready;
	IpcQt_TransthreadCaller						m_transthread_caller;
	std::weak_ptr<AutotestClient_Internals>		m_client_internals;
	std::weak_ptr<ServerInternals>				m_server_internals;

	TestabilityThreadQt							m_background_thread;
};
