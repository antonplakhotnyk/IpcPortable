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
	if( (MAssIpc_TransthreadTarget::CurrentThread() == m_transport_thread_id) 
	   || (MAssIpc_TransthreadTarget::DirectCallPseudoId() == m_transport_thread_id) )
		invoke_proc();
	else
	{
		std::unique_ptr<CallProc> call(std::make_unique<CallProc>(m_transport, invoke_proc));
		if(std::shared_ptr<MAssIpc_TransthreadCaller::CallWaiter> call_waiter = m_inter_thread->CallFromThread(m_transport_thread_id, std::move(call)))
			call_waiter->WaitProcessing();
	}
}

bool	IpcQt_TransthreadTransportCopy::WaitRespond(size_t expected_size)
{
	std::shared_ptr<std::atomic<bool>> result(std::make_shared<std::atomic<bool>>(false));
	auto invoke_proc = [=, result = result, transport = m_transport]()
	{
		result->store(transport->WaitRespond(expected_size));
	};

	CallFromThread(invoke_proc);
	return result->load();
}

size_t	IpcQt_TransthreadTransportCopy::ReadBytesAvailable()
{
	std::shared_ptr<std::atomic<size_t> > result(std::make_shared<std::atomic<size_t> >(0));
	auto invoke_proc = [result, transport = m_transport]()
	{
		result->store(transport->ReadBytesAvailable());
	};

	CallFromThread(invoke_proc);
	return result->load();
}

bool	IpcQt_TransthreadTransportCopy::Read(uint8_t* data, size_t size)
{
	std::shared_ptr<std::atomic<bool>> result(std::make_shared<std::atomic<bool>>(false));
	auto invoke_proc = [=, result = result, transport = m_transport]()
	{
		result->store(transport->Read(data, size));
	};

	CallFromThread(invoke_proc);
	return result->load();
}

void	IpcQt_TransthreadTransportCopy::Write(const uint8_t* data, size_t size)
{
	auto invoke_proc = [=, transport = m_transport]()
	{
		transport->Write(data, size);
	};

	CallFromThread(invoke_proc);
}
