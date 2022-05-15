#pragma once

#include "SutHandlerThread.h"
#include "ThreadCallerQt.h"
#include "IpcServerNet.h"


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

private:

	ThreadCallerQt	m_thread_caller;
	IpcServerNet	m_ipc_server;

	decltype(Params::autotest_container)		m_autotest_container;
	std::unique_ptr<SutHandlerThread>	m_sut_handler;
};
