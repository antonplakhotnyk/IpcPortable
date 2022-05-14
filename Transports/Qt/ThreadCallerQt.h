#pragma once

#include <QtCore/QThread>
#include <QtCore/QEvent>
#include <memory>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include "MAssIpcThreadTransportTarget.h"

class ThreadCallerQt: public QObject
{
public:
	ThreadCallerQt();
	~ThreadCallerQt();

	static MAssIpcThreadTransportTarget::Id	AddTargetThread(QThread* receiver);

	static MAssIpcThreadTransportTarget::Id	GetCurrentThreadId();
	static MAssIpcThreadTransportTarget::Id	GetId(QThread* thread);

public:

	struct WaitSync
	{
		std::condition_variable	condition;
		std::mutex mutex_sync;
	};

	class CallWaiter
	{
	public:

		CallWaiter(std::shared_ptr<WaitSync> wait_return_processing_calls)
			:m_wait_return_processing_calls(wait_return_processing_calls)
		{
		}

		void WaitProcessing();
		void CallDone();

	private:
		bool m_call_done = false;
		std::shared_ptr<WaitSync> m_wait_return_processing_calls;
	};



	class CallEvent: public QEvent
	{
	public:

		CallEvent():QEvent(QEvent::User){}

		void ProcessCallFromTargetThread();

		void SetCallWaiter(std::shared_ptr<CallWaiter> call_waiter);

	protected:
		virtual void CallFromTargetThread() = 0;

	protected:

		std::shared_ptr<CallWaiter> m_call_waiter;
	};


private:



	struct CallReceiver: public QObject
	{
		void customEvent(QEvent* event) override;
	};



	struct ThreadCallReceiver: public CallReceiver
	{
		void OnFinished_ReceiverThread();

		std::shared_ptr<WaitSync> GetWaitCallSync() const;

	private:

		std::shared_ptr<WaitSync> m_wait_return_processing_calls = std::make_shared<WaitSync>();
	};

public:

	void CallFromThread(MAssIpcThreadTransportTarget::Id thread_id, std::unique_ptr<CallEvent> call,
						std::shared_ptr<CallWaiter>* call_waiter);
protected:
	void CallNoThread(std::unique_ptr<CallEvent> call);
	
	static void ProcessCalls();

private:

	struct Internals
	{
		std::mutex	lock_thread_receivers;
		std::map<MAssIpcThreadTransportTarget::Id, std::unique_ptr<ThreadCallReceiver> > thread_receivers;
	};

	std::shared_ptr<Internals> m_int;
	static std::weak_ptr<Internals> s_int_inter_thread;
	static std::mutex	s_lock_int_inter_thread;

	CallReceiver	m_receiver_no_thread;
};

