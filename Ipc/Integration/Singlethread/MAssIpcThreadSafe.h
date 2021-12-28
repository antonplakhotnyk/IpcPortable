#pragma once

class MAssIpcThreadSafe
{
public:

	class mutex
	{
	public:
	};

	struct defer_lock_t
	{ // indicates defer lock
		explicit defer_lock_t() = default;
	};


	template<class TMutex>
	class unique_lock
	{
	public:

		unique_lock(TMutex& mtx)
		{
		}

		unique_lock(TMutex& _Mtx, defer_lock_t)
		{
		}

		void lock()
		{
		}

		void unlock()
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


	using id = size_t;

	static id get_id()
	{
		return 1;
	}
};
