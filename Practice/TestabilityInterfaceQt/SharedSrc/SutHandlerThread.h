#pragma once

#include "AutotestBase.h"
#include "IpcQt_Transthread.h"
#include "AutotestServer_Client.h"

class SutHandlerThread: public QThread
{
public:

	SutHandlerThread(std::shared_ptr<AutotestServer_Client::SutConnection> autotest_client_connection, TAutotestCreate autotest_container)
		:m_autotest_container(autotest_container)
		, m_autotest_client_connection(autotest_client_connection)
	{
		IpcQt_Transthread::AddTargetThread(this);
		start();
	}

	~SutHandlerThread()
	{
		requestInterruption();
		exit();
		IpcQt_TransthreadCaller::CancelDisableWaitingCall(IpcQt_TransthreadCaller::GetId(this));
		wait();
	}

	void run() override
	{
		while( true )
		{
			std::unique_ptr<AutotestBase> autotest(m_autotest_container(m_autotest_client_connection->ipc_net.ipc_call));
			if( !bool(autotest) )
				break;
			autotest->Run();
		}
	}

private:

	TAutotestCreate	m_autotest_container;
	std::shared_ptr<AutotestServer_Client::SutConnection> m_autotest_client_connection;
};