#pragma once

#include "MAssCallThreadTransport.h"
#include "ThreadCallerQt.h"

class IpcThreadTransportQt: public MAssCallThreadTransport, public ThreadCallerQt
{
public:

	IpcThreadTransportQt();
	~IpcThreadTransportQt();

private:

	void			CallFromThread(MAssIpcThreadTransportTarget::Id thread_id, std::unique_ptr<Job> job) override;
	MAssIpcThreadTransportTarget::Id	GetResultSendThreadId() override;

private:

	struct JobEvent: public ThreadCallerQt::CallEvent
	{
		JobEvent(std::unique_ptr<MAssCallThreadTransport::Job> job): m_job(std::move(job)){}
	private:
		void CallFromTargetThread() override;
	private:
		std::unique_ptr<MAssCallThreadTransport::Job> m_job;
	};

};
