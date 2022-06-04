#pragma once

#include "MAssIpc_TransthreadTarget.h"
#include <memory>

class MAssIpc_Transthread
{
public:

	class Job
	{
	public:
		virtual ~Job() = default;
		virtual void Invoke() = 0;
	};

	static inline MAssIpc_TransthreadTarget::Id NoThread()
	{
		return MAssIpc_TransthreadTarget::Id();
	}

	virtual void			CallFromThread(MAssIpc_TransthreadTarget::Id thread_id, std::unique_ptr<Job> job) = 0;
	virtual MAssIpc_TransthreadTarget::Id	GetResultSendThreadId()=0;
};

