#pragma once

#include "SutHandlerThread.h"
#include "IpcQt_TransthreadCaller.h"
#include "IpcQt_NetServer.h"


class AutotestServer: public QObject
{
public:

	class Client
	{
	public:

		struct Handlers
		{
			std::function<void()> OnConnectedSut;
			std::function<void()> OnDisconnectedSut;
		};

		Client(const Handlers& handlers, const MAssIpcCall& ipc_connection)
			:m_ipc_connection(ipc_connection)
			, m_handlers(handlers)
		{
		}

		const MAssIpcCall& IpcCall() const {return m_ipc_connection;}

	protected:

		MAssIpcCall		m_ipc_connection;
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
	};


	void OnConnected();
	void OnDisconnected();
	static void IpcError(MAssIpcCall::ErrorType et, std::string message);

private:

	IpcQt_TransthreadCaller	m_thread_caller;
	std::shared_ptr<IpcQt_TransportTcpServer> m_transport_server;
	MAssIpcCall		m_ipc_connection;
	IpcQt_Net			m_ipc_net;

	decltype(Params::autotest_container)		m_autotest_container;
	std::unique_ptr<SutHandlerThread>			m_sut_handler;


	std::vector<std::weak_ptr<ClientPrivate>>	m_clients;
};
