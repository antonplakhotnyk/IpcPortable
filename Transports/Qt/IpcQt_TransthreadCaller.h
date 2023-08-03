#pragma once

#include <QtCore/QThread>
#include <QtCore/QEvent>
#include "MAssIpc_TransthreadTarget.h"
#include <memory>
#include <mutex>
#include <condition_variable>
#include <set>

class IpcQt_TransthreadCaller: public QObject
{
public:
	IpcQt_TransthreadCaller();
	~IpcQt_TransthreadCaller();

	static MAssIpc_TransthreadTarget::Id	AddTargetThread(QThread* sender_or_receiver);

	static MAssIpc_TransthreadTarget::Id	GetCurrentThreadId();
	static MAssIpc_TransthreadTarget::Id	GetId(QThread* thread);
	static void CancelDisableWaitingCall(MAssIpc_TransthreadTarget::Id thread_waiting_call);

	class CallWaiter
	{
	public:

		virtual ~CallWaiter() = default;

		enum CallState: uint8_t
		{
			cs_no_call,
			cs_inprogress,
			cs_canceled,
			cs_done,
			cs_receiver_thread_finished
		};

		virtual CallState WaitProcessing()
		{
			return cs_no_call;
		}
	};

	class CallEvent;

	std::shared_ptr<CallWaiter> CallFromThread(MAssIpc_TransthreadTarget::Id receiver_thread_id, std::unique_ptr<CallEvent> call);

protected:

	static void ProcessCalls();

private:

	struct WaitSync
	{
		std::condition_variable	condition;
		std::mutex mutex_sync;
	};

	class CallWaiterPrivate: public CallWaiter
	{
	public:

		CallWaiterPrivate(std::shared_ptr<std::set<CallWaiterPrivate*> > sender_calls,
						  std::shared_ptr<std::set<CallWaiterPrivate*> > receiver_calls,
						  std::shared_ptr<WaitSync> wait_return_processing_calls)
			:m_wait_return_processing_calls(wait_return_processing_calls)
			, m_sender_calls(sender_calls)
			, m_receiver_calls(receiver_calls)
		{
			if( m_sender_calls )
				m_sender_calls->insert(this);
			if( m_receiver_calls )
				m_receiver_calls->insert(this);
		}

		~CallWaiterPrivate()
		{
			if( m_sender_calls )
				m_sender_calls->erase(this);
			if( m_receiver_calls )
				m_receiver_calls->erase(this);
		}

		CallState WaitProcessing() override;

		void CallDone();
		void CallCancel();
		void SetReceiverThreadFinished();

	private:
		CallState m_call_state = cs_inprogress;
		std::shared_ptr<WaitSync> m_wait_return_processing_calls;

		const std::shared_ptr<std::set<CallWaiterPrivate*> > m_sender_calls;
		const std::shared_ptr<std::set<CallWaiterPrivate*> > m_receiver_calls;
	};

public:

	class CallEvent: public QEvent
	{
	public:

		CallEvent():QEvent(QEvent::User){}

		void ProcessCallFromTargetThread();

		void SetCallWaiter(std::shared_ptr<CallWaiterPrivate> call_waiter);

	protected:
		virtual void CallFromTargetThread() = 0;

	protected:

		std::shared_ptr<CallWaiterPrivate> m_call_waiter;
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

		std::shared_ptr<std::set<CallWaiterPrivate*> > m_call_waiters_sender = std::make_shared<std::set<CallWaiterPrivate*>>();
		std::shared_ptr<std::set<CallWaiterPrivate*> > m_call_waiters_receiver = std::make_shared<std::set<CallWaiterPrivate*>>();

	private:
		const std::shared_ptr<WaitSync> m_sender_wait_return_receiver_processing_calls = std::make_shared<WaitSync>();
	};


private:

	struct Internals
	{
		std::mutex	lock_threads;
		std::map<MAssIpc_TransthreadTarget::Id, std::unique_ptr<ThreadCallReceiver> > threads;
	};

	std::shared_ptr<Internals> m_int;
	static std::weak_ptr<Internals> s_int_inter_thread;
	static std::mutex	s_lock_int_inter_thread;

	CallReceiver	m_receiver_no_thread;
};

