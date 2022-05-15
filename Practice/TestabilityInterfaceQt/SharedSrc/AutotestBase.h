#pragma once

#include "MAssIpcCall.h"

class AutotestBase
{
public:
	AutotestBase(MAssIpcCall& sut_ipc)
		:m_sut_ipc(sut_ipc)
	{
	}

	virtual ~AutotestBase()
	{
		m_sut_ipc.ClearHandlersWithTag(Tag());
	}

	const void* Tag() const
	{
		return this;
	}

	virtual void Run() = 0;

protected:

	MAssIpcCall m_sut_ipc;
};

//-------------------------------------------------------
// class AutotestContainer
// {
// public:
// 
// 	virtual size_t			GetIndexCount() = 0;
// 	virtual std::unique_ptr<AutotestBase>	Create(size_t index, MAssIpcCall& sut_ipc) = 0;
// };

// using TAutotestContainer = std::shared_ptr<AutotestContainer>;
using TAutotestCreate = std::function<std::unique_ptr<AutotestBase>(MAssIpcCall& sut_ipc)>;
