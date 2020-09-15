#pragma once

#include <mutex>
#include <condition_variable>

class MAssIpcThreadSafe
{
public:

	using mutex = std::mutex;
	using condition_variable = std::condition_variable;
	template<class TMutex>
	using unique_lock = std::unique_lock<TMutex>;
};
