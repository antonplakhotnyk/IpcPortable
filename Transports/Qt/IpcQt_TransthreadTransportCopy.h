#pragma once

#include "MAssIpc_Transport.h"
#include "MAssIpc_Transthread.h"
#include "MAssIpc_TransthreadTarget.h"
#include "IpcQt_TransthreadCaller.h"

class IpcQt_TransthreadTransportCopy: public MAssIpc_TransportCopy, public QObject
{
public:

	IpcQt_TransthreadTransportCopy(MAssIpc_TransthreadTarget::Id transport_thread_id, 
							 const std::shared_ptr<MAssIpc_TransportCopy>& transport, 
							 const std::shared_ptr<IpcQt_TransthreadCaller>& inter_thread);

	bool	WaitRespond(size_t expected_size) override;

	size_t	ReadBytesAvailable() override;
	void	Read(uint8_t* data, size_t size) override;
	void	Write(const uint8_t* data, size_t size) override;

private:

	void CallFromThread(std::function<void()> invoke_proc);

	class Call: public IpcQt_TransthreadCaller::CallEvent
	{
	public:
		Call(std::shared_ptr<MAssIpc_TransportCopy> transport, std::function<void()> invoke_proc)
			:m_invoke_proc(invoke_proc)
		{
		}

	private:
		void CallFromTargetThread() override
		{
			m_invoke_proc();
		}

	private:
		std::function<void()> m_invoke_proc;
	};

private:

	std::shared_ptr<MAssIpc_TransportCopy>	m_transport;
	std::shared_ptr<IpcQt_TransthreadCaller>			m_inter_thread;
	const MAssIpc_TransthreadTarget::Id	m_transport_thread_id;
};