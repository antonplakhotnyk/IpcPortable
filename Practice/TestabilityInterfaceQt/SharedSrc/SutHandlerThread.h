#pragma once

#include "AutotestBase.h"
#include "IpcThreadTransportQt.h"

class SutHandlerThread: public QThread
{
public:

	SutHandlerThread(MAssIpcCall& sut_ipc, TAutotestCreate autotest_container)
		:m_autotest_container(autotest_container)
		, m_sut_ipc(sut_ipc)
	{
		IpcThreadTransportQt::AddTargetThread(this);
		start();
	}

	~SutHandlerThread()
	{
		requestInterruption();
		exit();
		ThreadCallerQt::CancelDisableWaitingCall(ThreadCallerQt::GetId(this));
		wait();
	}

	void run() override
	{
		while( true )
		{
			std::unique_ptr<AutotestBase> autotest(m_autotest_container(m_sut_ipc));
			if( !bool(autotest) )
				break;
			autotest->Run();
		}
	}

private:

	TAutotestCreate	m_autotest_container;
	MAssIpcCall		m_sut_ipc;
};