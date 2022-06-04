#pragma once

#include "MAssIpcThreadTransportTarget.h"
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

	static inline MAssIpcThreadTransportTarget::Id NoThread()
	{
		return MAssIpcThreadTransportTarget::Id();
	}

	virtual void			CallFromThread(MAssIpcThreadTransportTarget::Id thread_id, std::unique_ptr<Job> job) = 0;
	virtual MAssIpcThreadTransportTarget::Id	GetResultSendThreadId()=0;
};

