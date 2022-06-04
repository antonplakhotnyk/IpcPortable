#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

class MAssIpc_ThreadSafe
{
public:

	using mutex = std::mutex;
	using condition_variable = std::condition_variable;
	template<class TMutex>
	using unique_lock = std::unique_lock<TMutex>;
	using defer_lock_t = std::defer_lock_t;
	using atomic_uint32_t = std::atomic<uint32_t>;

	using id = std::thread::id;

	static inline id get_id()
	{
		return std::this_thread::get_id();
	}
};
