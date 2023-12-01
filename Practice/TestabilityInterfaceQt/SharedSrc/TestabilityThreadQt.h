#pragma once

#include <QtCore/QThread>
#include <tuple>

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

//-------------------------------------------------------

class TestabilityWaitReady
{
	bool m_ready = false;
	std::condition_variable	m_condition;
	std::mutex m_mutex;

public:

	void Wait()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		while( !m_ready )
			m_condition.wait(lock);
	}

	void SetReady()
	{
		if( !m_ready )
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			m_ready=true;
			m_condition.notify_all();
		}
	}
};


//-------------------------------------------------------
