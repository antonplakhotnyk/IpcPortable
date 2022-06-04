#pragma once

#include "MAssIpc_Transthread.h"
#include "IpcQt_TransthreadCaller.h"

class IpcQt_Transthread: public MAssIpc_Transthread, public IpcQt_TransthreadCaller
{
public:

	IpcQt_Transthread();
	~IpcQt_Transthread();

private:

	void			CallFromThread(MAssIpc_TransthreadTarget::Id thread_id, std::unique_ptr<Job> job) override;
	MAssIpc_TransthreadTarget::Id	GetResultSendThreadId() override;

private:

	struct JobEvent: public IpcQt_TransthreadCaller::CallEvent
	{
		JobEvent(std::unique_ptr<MAssIpc_Transthread::Job> job): m_job(std::move(job)){}
	private:
		void CallFromTargetThread() override;
	private:
		std::unique_ptr<MAssIpc_Transthread::Job> m_job;
	};

};
