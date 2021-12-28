#pragma once

#include "IThreadCallTransport.h"
#include "ThreadCallerQt.h"

class IpcThreadTransportQt: public IThreadCallTransport, public ThreadCallerQt
{
public:

	IpcThreadTransportQt();
	~IpcThreadTransportQt();

private:

	void			CallFromThread(AVThread::Id thread_id, Job* job) override;
	AVThread::Id	GetCurrentThreadId() override;

private:

	struct JobEvent: public ThreadCallerQt::CallEvent
	{
		JobEvent(IThreadCallTransport::Job* job): m_job(job){}
	private:
		void CallFromTargetThread() override;
	private:
		OPtr<IThreadCallTransport::Job> m_job;
	};

};
