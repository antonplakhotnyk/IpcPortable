#pragma once

#include "MAssIpc_ThreadSafe.h"


// This class only shows interface required by library.
// you responsible for implementing this interface according to your needs.
//
// Thread transport id may be implemented not same as std::threads::id and may be not same as MAssIpc_ThreadSafe::Id
class MAssIpc_TransthreadTarget
{
public:

	class Id
	{
	public:

		constexpr Id() = default;

	private:
		size_t m_id = -1;// no thread
	};

	static Id CurrentThread()
	{
		return Id();
	}

	// possible implementations:
	// 
	// Same as MAssIpc_ThreadSafe
	// using Id = MAssIpc_ThreadSafe::Id;
	// static Id CurrentThread()
	// {
	// 	return MAssIpc_ThreadSafe::get_id();
	// }
	//
	// based on std::thread
	// using Id = std::thread::id
	// static inline id get_id()
	// {
	// 	return std::this_thread::get_id();
	// }

};
