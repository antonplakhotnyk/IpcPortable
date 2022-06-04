#pragma once

#include "SutHandlerThread.h"
#include "IpcQt_TransthreadCaller.h"
#include "IpcQt_NetServer.h"


class AutotestServer: public QObject
{
public:

	struct Params
	{
		TAutotestCreate autotest_container;
		uint16_t server_port = 2233;
	};

	AutotestServer(const Params& params);

private:

	void OnConnected();
	void OnDisconnected();
	static void IpcError(MAssIpcCall::ErrorType et, std::string message);

private:

	IpcQt_TransthreadCaller	m_thread_caller;
	std::shared_ptr<IpcQt_TransportTcpServer> m_transport_server;
	MAssIpcCall		m_ipc_connection;
	IpcQt_Net			m_ipc_net;

	decltype(Params::autotest_container)		m_autotest_container;
	std::unique_ptr<SutHandlerThread>	m_sut_handler;
};
