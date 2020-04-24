#pragma once

#include "Integration/MAssThread.h"
#include <memory>

class MAssCallThreadTransport
{
public:

	class Job
	{
	public:
		virtual void Invoke() = 0;
	};

	virtual void			CallFromThread(MAssThread::Id thread_id, const std::shared_ptr<Job>& job) = 0;
	virtual MAssThread::Id	GetCurrentThreadId()=0;
};

