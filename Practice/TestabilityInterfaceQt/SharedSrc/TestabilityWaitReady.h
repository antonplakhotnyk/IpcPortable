#pragma once

#include <mutex>
#include <condition_variable>

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
			m_ready = true;
			m_condition.notify_all();
		}
	}
};
