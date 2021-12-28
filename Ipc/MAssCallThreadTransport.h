#pragma once

#include "MAssIpcThread.h"
#include <memory>

class MAssCallThreadTransport
{
public:

	class Job
	{
	public:
		virtual ~Job() = default;
		virtual void Invoke() = 0;
	};

	virtual void			CallFromThread(MAssIpcThread::Id thread_id, std::unique_ptr<Job> job) = 0;
	virtual MAssIpcThread::Id	GetResultSendThreadId()=0;
};

