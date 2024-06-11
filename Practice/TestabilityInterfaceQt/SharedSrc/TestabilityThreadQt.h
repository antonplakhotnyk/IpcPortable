#pragma once

#include <QtCore/QThread>
#include <tuple>
#include "IpcQt_TransthreadCaller.h"

class TestabilityThreadQt: public QThread
{
public:

	TestabilityThreadQt(QObject* parent, std::function<void()>&& main_proc)
		: QThread(parent)
		, m_main_proc(std::move(main_proc))
	{
	}

	~TestabilityThreadQt()
	{
		Stop();
	}

	void run() override
	{
		IpcQt_TransthreadCaller::AddTargetThread(this);
		m_main_proc();
	}

	void Stop()
	{
		if( this->isRunning() )
		{
			this->requestInterruption();
			this->exit();
			IpcQt_TransthreadCaller::CancelDisableWaitingCall(IpcQt_TransthreadCaller::GetId(this));
			this->wait();
		}
	}

private:

	std::function<void()> m_main_proc;
};

