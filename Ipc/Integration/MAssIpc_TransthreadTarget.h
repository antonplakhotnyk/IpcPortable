#pragma once

#include "MAssIpc_ThreadSafe.h"


// This class only shows interface required by library.
// you responsible for implementing this interface according to your needs.
//
// Thread transport id may be implemented not same as std::threads::id and may be not same as MAssIpcThreadSafe::Id
class MAssIpc_TransthreadTarget
{
public:

	class Id
	{
	public:

		constexpr Id() = default;

	private:

		size_t m_id = -1;
	};

	// possible implementations:
	//using Id = MAssIpcThreadSafe::Id;
	//using Id = std::thread::id
};
