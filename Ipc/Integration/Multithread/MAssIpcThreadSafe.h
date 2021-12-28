#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>

class MAssIpcThreadSafe
{
public:

	using mutex = std::mutex;
	using condition_variable = std::condition_variable;
	template<class TMutex>
	using unique_lock = std::unique_lock<TMutex>;
	using defer_lock_t = std::defer_lock_t;

	using id = std::thread::id;

	static id get_id()
	{
		return std::this_thread::get_id();
	}
};
