#include "IpcQt_TransthreadTransportCopy.h"
#include "IpcQt_TransthreadCaller.h"

IpcQt_TransthreadTransportCopy::IpcQt_TransthreadTransportCopy(MAssIpc_TransthreadTarget::Id transport_thread_id,
												   const std::shared_ptr<MAssIpc_TransportCopy>& transport,
												   const std::shared_ptr<IpcQt_TransthreadCaller>& inter_thread)
	:m_transport(transport)
	, m_inter_thread(inter_thread)
	, m_transport_thread_id(transport_thread_id)
{
}

void IpcQt_TransthreadTransportCopy::CallFromThread(std::function<void()> invoke_proc)
{
	if( IpcQt_TransthreadCaller::GetCurrentThreadId() == m_transport_thread_id )
		invoke_proc();
	else
	{
		std::unique_ptr<Call> call(std::make_unique<Call>(m_transport, invoke_proc));
		std::shared_ptr<IpcQt_TransthreadCaller::CallWaiter> call_waiter = m_inter_thread->CallFromThread(m_transport_thread_id, std::move(call));

		call_waiter->WaitProcessing();
	}
}

bool	IpcQt_TransthreadTransportCopy::WaitRespond(size_t expected_size)
{
	std::shared_ptr<bool> result(std::make_shared<bool>(false));
	auto invoke_proc = [=, result = result, transport = m_transport]()
	{
		*result.get() = transport->WaitRespond(expected_size);
	};

	CallFromThread(invoke_proc);
	return *result.get();
}

size_t	IpcQt_TransthreadTransportCopy::ReadBytesAvailable()
{
	std::shared_ptr<std::atomic<size_t> > result(std::make_shared<std::atomic<size_t> >(false));
	auto invoke_proc = [result, transport = m_transport]()
	{
		result->store(transport->ReadBytesAvailable());
	};

	CallFromThread(invoke_proc);
	return result->load();
}

void	IpcQt_TransthreadTransportCopy::Read(uint8_t* data, size_t size)
{
	auto invoke_proc = [=, transport = m_transport]()
	{
		transport->Read(data, size);
	};

	CallFromThread(invoke_proc);
}

void	IpcQt_TransthreadTransportCopy::Write(const uint8_t* data, size_t size)
{
	auto invoke_proc = [=, transport = m_transport]()
	{
		transport->Write(data, size);
	};

	CallFromThread(invoke_proc);
}
