#pragma once

#include "MAssIpc_Transthread.h"

#include <mutex>
#include <condition_variable>
#include <queue>

class IpcTransthread_Memory: public MAssIpc_Transthread
{
public:

	void CallFromThread(MAssIpc_TransthreadTarget::Id thread_id, std::unique_ptr<Job> job) override
	{
		if( !(thread_id<MAssIpc_TransthreadTarget::Id()) && !(MAssIpc_TransthreadTarget::Id()<thread_id) )
			job->Invoke();
		else
		{
			std::unique_lock<std::mutex> lock(m_mutex_job_queue);
 			m_job_queue[thread_id].push(std::move(job));
			m_cond_var_job_queue.notify_all();
		}
	}

	std::unique_ptr<MAssIpc_Transthread::Job> WaitFromThread(MAssIpc_TransthreadTarget::Id thread_id = MAssIpc_TransthreadTarget::CurrentThread())
	{
		std::unique_lock<std::mutex> lock(m_mutex_job_queue);
		auto& queue_jobs = m_job_queue[thread_id];

		m_cond_var_job_queue.wait(lock, [&]
		{
			if( m_cancel_wait )
				return true;
			return !queue_jobs.empty();
		});

		if( queue_jobs.empty() )
			return {};

		auto job = std::move(queue_jobs.front());
		queue_jobs.pop();
		return job;
	}

	MAssIpc_TransthreadTarget::Id GetResultSendThreadId() override
	{
		return MAssIpc_TransthreadTarget::Id();
	}

	void CancelWait()
	{
		std::unique_lock<std::mutex> lock(m_mutex_job_queue);
		m_cancel_wait = true;
		m_cond_var_job_queue.notify_all();

	}

private:

	bool m_cancel_wait = false;
	std::map<MAssIpc_TransthreadTarget::Id, std::queue<std::unique_ptr<MAssIpc_Transthread::Job>>>	 m_job_queue;

	std::mutex m_mutex_job_queue;
	std::condition_variable m_cond_var_job_queue;
};

