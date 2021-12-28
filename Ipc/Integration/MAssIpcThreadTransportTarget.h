#pragma once

#include "MAssIpcThreadSafe.h"


class MAssIpcThreadTransportTarget
{
public:

	// Thread transport may be implemented not based on std::threads used thread-id type which differ from MAssIpcThreadTransportTarget::Id
	//using Id = MAssIpcThreadSafe::Id;

	class Id
	{
	public:

		constexpr Id() = default;

		// used only for tests
		constexpr Id(size_t id)
			:m_id(id)
		{
		}

	private:

		size_t m_id = -1;
	};
};
