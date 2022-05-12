#include "CallTransportFromQThread.h"
#include "ThreadCallerQt.h"

CallTransportFromQThread::CallTransportFromQThread(MAssIpcThreadTransportTarget::Id transport_thread_id,
												   const std::shared_ptr<MAssIpcCallTransport>& transport,
												   const std::shared_ptr<ThreadCallerQt>& inter_thread)
	:m_transport(transport)
	, m_inter_thread(inter_thread)
	, m_transport_thread_id(transport_thread_id)
{
}

void CallTransportFromQThread::CallFromThread(std::function<void()> invoke_proc)
{
	if( ThreadCallerQt::GetCurrentThreadId() == m_transport_thread_id )
		invoke_proc();
	else
	{
		std::shared_ptr<ThreadCallerQt::CallWaiter> call_waiter;
		std::unique_ptr<Call> call(std::make_unique<Call>(m_transport, invoke_proc));
		m_inter_thread->CallFromThread(m_transport_thread_id, std::move(call), &call_waiter);

		call_waiter->WaitProcessing();
	}
}

bool	CallTransportFromQThread::WaitRespond(size_t expected_size)
{
	bool result = false;
	auto invoke_proc = [=, &result, transport = m_transport]()
	{
		result = transport->WaitRespond(expected_size);
	};

	CallFromThread(invoke_proc);
	return result;
}

size_t	CallTransportFromQThread::ReadBytesAvailable()
{
	size_t result = 0;
	auto invoke_proc = [&result, transport = m_transport]()
	{
		result = transport->ReadBytesAvailable();
	};

	CallFromThread(invoke_proc);
	return result;
}

void	CallTransportFromQThread::Read(uint8_t* data, size_t size)
{
	auto invoke_proc = [=, transport = m_transport]()
	{
		transport->Read(data, size);
	};

	CallFromThread(invoke_proc);
}

void	CallTransportFromQThread::Write(const uint8_t* data, size_t size)
{
	auto invoke_proc = [=, transport = m_transport]()
	{
		transport->Write(data, size);
	};

	CallFromThread(invoke_proc);
}
