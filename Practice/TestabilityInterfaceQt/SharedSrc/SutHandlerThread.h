#pragma once

#include "AutotestBase.h"
#include "IpcQt_Transthread.h"

class SutHandlerThread: public QThread
{
public:

	SutHandlerThread(MAssIpcCall& sut_ipc, TAutotestCreate autotest_container)
		:m_autotest_container(autotest_container)
		, m_sut_ipc(sut_ipc)
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