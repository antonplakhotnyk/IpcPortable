#pragma once

class MAssIpcThreadSafe
{
public:

	class mutex
	{
	};

	template<class TMutex>
	class unique_lock
	{
	public:

		unique_lock(TMutex& mtx)
		{
		}
	};

	class condition_variable
	{
	public:

		void wait(unique_lock<mutex>& lck)
		{
		}

		void notify_all()
		{
		}
	};
};
