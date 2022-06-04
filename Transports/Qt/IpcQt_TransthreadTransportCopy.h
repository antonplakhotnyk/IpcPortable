#pragma once

#include "MAssIpcCallTransport.h"
#include "MAssCallThreadTransport.h"
#include "MAssIpcThreadTransportTarget.h"
#include "ThreadCallerQt.h"

class CallTransportFromQThread: public MAssIpcCallTransport, public QObject
{
public:

	CallTransportFromQThread(MAssIpcThreadTransportTarget::Id transport_thread_id, 
							 const std::shared_ptr<MAssIpcCallTransport>& transport, 
							 const std::shared_ptr<ThreadCallerQt>& inter_thread);

	bool	WaitRespond(size_t expected_size) override;

	size_t	ReadBytesAvailable() override;
	void	Read(uint8_t* data, size_t size) override;
	void	Write(const uint8_t* data, size_t size) override;

private:

	void CallFromThread(std::function<void()> invoke_proc);

	class Call: public ThreadCallerQt::CallEvent
	{
	public:
		Call(std::shared_ptr<MAssIpcCallTransport> transport, std::function<void()> invoke_proc)
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

	std::shared_ptr<MAssIpcCallTransport>	m_transport;
	std::shared_ptr<ThreadCallerQt>			m_inter_thread;
	const MAssIpcThreadTransportTarget::Id	m_transport_thread_id;
};