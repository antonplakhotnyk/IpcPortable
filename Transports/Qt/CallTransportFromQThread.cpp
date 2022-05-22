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
		std::unique_ptr<ThreadCallerQt::CancelHolder> call_waiter;
		std::unique_ptr<Call> call(std::make_unique<Call>(m_transport, invoke_proc));
		m_inter_thread->CallFromThread(m_transport_thread_id, std::move(call), &call_waiter);

		call_waiter->m_call_waiter_cancel->WaitProcessing();
	}
}

bool	CallTransportFromQThread::WaitRespond(size_t expected_size)
{
	std::shared_ptr<bool> result(std::make_shared<bool>(false));
	auto invoke_proc = [=, result = result, transport = m_transport]()
	{
		*result.get() = transport->WaitRespond(expected_size);
	};

	CallFromThread(invoke_proc);
	return *result.get();
}

size_t	CallTransportFromQThread::ReadBytesAvailable()
{
	std::shared_ptr<std::atomic<size_t> > result(std::make_shared<std::atomic<size_t> >(false));
	auto invoke_proc = [result, transport = m_transport]()
	{
		result->store(transport->ReadBytesAvailable());
	};

	CallFromThread(invoke_proc);
	return result->load();
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
