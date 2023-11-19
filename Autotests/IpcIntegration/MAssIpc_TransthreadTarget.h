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

		constexpr Id()=default;

		constexpr bool operator<(const Id& other) const
		{
			return m_id < other.m_id;
		}

		constexpr bool operator==(const Id& other) const
		{
			return m_id == other.m_id;
		}

	private:
		size_t m_id = -1;// no thread
	};

	static Id CurrentThread()
	{
		return Id();
	}

	static Id DirectCallPseudoId()
	{
		return CurrentThread();
	}

	// Possible implementations:
	 
// 	// Same as MAssIpc_ThreadSafe
// 	using Id = MAssIpc_ThreadSafe::id;
// 	static Id CurrentThread()
// 	{
// 		return MAssIpc_ThreadSafe::get_id();
// 	}
// 
// 	static Id DirectCallPseudoId()
// 	{
// 		return Id();
// 	}

// 	// based on std::thread
// 	using Id = std::thread::id;
// 	static inline Id CurrentThread()
// 	{
// 		return std::this_thread::get_id();
// 	}
// 
// 	static Id DirectCallPseudoId()
// 	{
// 		return Id();
// 	}

};
