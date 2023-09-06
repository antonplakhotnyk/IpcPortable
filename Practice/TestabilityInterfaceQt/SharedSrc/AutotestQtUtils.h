#pragma once

#include "MAssIpcCall.h"
#include <QtCore/QEventLoop>

class AutotestQtUtils
{
public:

	static bool WaitProcessingSutReady(MAssIpcCall& sut_ipc, QEventLoop& event_loop);
	static bool WaitIncomingCall(const char* proc_name, MAssIpcCall& sut_ipc, QEventLoop& event_loop);
	static size_t WaitIncomingCalls(std::vector<const char*> proc_names, MAssIpcCall& sut_ipc, QEventLoop& event_loop);

	class Waiter
	{
	public:

		Waiter(const std::shared_ptr<const MAssIpcCall::CallInfo>& call_info, MAssIpcCall& sut_ipc, uint32_t initial_count)
			: m_sut_ipc(sut_ipc)
			, m_call_count(initial_count)
			, m_call_info(call_info)
		{
		}


		Waiter(const std::shared_ptr<const MAssIpcCall::CallInfo>& call_info, MAssIpcCall& sut_ipc)
			:Waiter(call_info, sut_ipc, call_info->GetCallCount())
		{
		}

		bool WaitProcessing(QEventLoop& event_loop)
		{
			MAssIpcCall::SetCallCountChangedGuard set_handler_guard(m_sut_ipc, std::bind(&Waiter::OnCallCountChanged, this, std::placeholders::_1, std::ref(event_loop)));

			if( IsCheckFinished_Autoreset() )
				return true;

			event_loop.exec();

			if( IsCheckFinished_Autoreset() )
				return true;

			return false;
		}

	private:

		bool IsCheckFinished_Autoreset()
		{
			const auto new_count = m_call_info->GetCallCount();
			if( new_count != m_call_count )
			{
				m_call_count = new_count;
				return true;
			}

			return false;
		}

		void OnCallCountChanged(std::shared_ptr<const MAssIpcCall::CallInfo> call_info, QEventLoop& event_loop)
		{
			if( m_call_info==call_info )
			{
				if( IsCheckFinished_Autoreset() )
					event_loop.exit();
			}
		}

	private:

		uint32_t m_call_count;
		MAssIpcCall m_sut_ipc;
		std::shared_ptr<const MAssIpcCall::CallInfo> m_call_info;
	};

};