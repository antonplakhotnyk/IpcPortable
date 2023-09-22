#pragma once

#include "SutHandlerThread.h"
#include "IpcQt_TransthreadCaller.h"
#include "IpcQt_TransportTcpServer.h"
#include "IpcQt_Net.h"
#include "MAssIpcWaiter.h"

class AutotestServer: public IpcQt_TransportTcpServer
{
public:

	struct Params
	{
		TAutotestCreate autotest_container;
		uint16_t server_port = 2233;
	};

	AutotestServer(const Params& params);

	std::shared_ptr<AutotestServer_Client> CreateClient(const AutotestServer_Client::Handlers& handlers);
	void		Stop();

private:

	class ClientPrivate: public AutotestServer_Client
	{
	public:
		template<class... Args>
		ClientPrivate(Args&&... args):AutotestServer_Client(args...){};

		const Handlers& GetHandlers() const {return m_handlers;}
		void SetConnection(SutIndexId sut_id, std::shared_ptr<AutotestServer_Client::SutConnection> connection);
	};


	void OnConnected(std::weak_ptr<IpcQt_TransporTcp> transport) override;
	void OnDisconnected(std::weak_ptr<IpcQt_TransporTcp> transport) override;

private:

//	std::set<std::shared_ptr<AutotestServer_Client::SutConnection>>	m_connections;

	decltype(Params::autotest_container)		m_autotest_container;
	std::unique_ptr<SutHandlerThread>			m_sut_handler;


	std::vector<std::weak_ptr<ClientPrivate>>	m_clients;
	std::shared_ptr<AutotestServerInternals>	m_server_internals = std::make_shared<AutotestServerInternals>();

};
