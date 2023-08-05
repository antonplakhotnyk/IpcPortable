#pragma once

#include <QtCore/QThread>
#include <QtCore/QEvent>
#include "MAssIpc_TransthreadTarget.h"
#include "IpcQtBind_TransthreadCaller.h"
#include <memory>
#include <mutex>
#include <condition_variable>
#include <set>

class IpcQt_TransthreadCaller: public IpcQtBind_TransthreadCaller
{
	class CallWaiterPrivate;

public:
	IpcQt_TransthreadCaller();
	~IpcQt_TransthreadCaller();

	static MAssIpc_TransthreadTarget::Id	AddTargetThread(QThread* sender_or_receiver);

	static void CancelDisableWaitingCall(MAssIpc_TransthreadTarget::Id thread_waiting_call);

	class CallEvent;
	class CallWaiter;

	std::shared_ptr<CallWaiter> CallFromThread(MAssIpc_TransthreadTarget::Id receiver_thread_id, std::unique_ptr<CallEvent> call);

public:

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

	class CallWaiterPrivate;

	class CallEvent: public QEvent
	{
	public:

		CallEvent():QEvent(QEvent::User)
		{
		}

		void ProcessCallFromTargetThread();

		void SetCallWaiter(std::shared_ptr<CallWaiterPrivate> call_waiter);

	protected:
		virtual void CallFromTargetThread() = 0;

	protected:

		std::shared_ptr<CallWaiterPrivate> m_call_waiter;
	};

protected:

	static void ProcessCalls();

private:

	struct WaitSync
	{
		void ProcessIncomingCall();

		bool process_incoming_call = false;
		std::condition_variable_any	condition;
		std::recursive_mutex mutex_sync;
	};

	class CallWaiterPrivate: public CallWaiter
	{
	public:

		CallWaiterPrivate(std::shared_ptr<std::set<CallWaiterPrivate*> > sender_calls,
						  std::shared_ptr<std::set<CallWaiterPrivate*> > receiver_calls,
						  std::shared_ptr<WaitSync> wait_return_processing_calls,
						  const std::shared_ptr<std::recursive_mutex>& lock_threads)
			:m_wait_return_processing_calls(wait_return_processing_calls)
			, m_sender_calls(sender_calls)
			, m_receiver_calls(receiver_calls)
			, lock_threads(lock_threads)
		{
			std::unique_lock<std::recursive_mutex> lock(*this->lock_threads.get());
			if( m_sender_calls )
				m_sender_calls->insert(this);
			if( m_receiver_calls )
				m_receiver_calls->insert(this);
		}

		~CallWaiterPrivate()
		{
			std::unique_lock<std::recursive_mutex> lock(*this->lock_threads.get());
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
		const std::shared_ptr<std::recursive_mutex>	lock_threads;
	};



private:


	struct ThreadCallReceiver:public QObject
	{
		void customEvent(QEvent* event) override;
		void OnFinished_ReceiverThread();

		std::shared_ptr<WaitSync> GetWaitCallSync() const;

		std::shared_ptr<std::set<CallWaiterPrivate*> > m_call_waiters_sender = std::make_shared<std::set<CallWaiterPrivate*>>();
		std::shared_ptr<std::set<CallWaiterPrivate*> > m_call_waiters_receiver = std::make_shared<std::set<CallWaiterPrivate*>>();

	private:
		const std::shared_ptr<WaitSync> m_sender_wait_return_receiver_processing_calls = std::make_shared<WaitSync>();
	};

	struct Internals
	{
		const std::shared_ptr<std::recursive_mutex>	lock_threads = std::make_shared<std::recursive_mutex>();
		std::map<MAssIpc_TransthreadTarget::Id, std::shared_ptr<ThreadCallReceiver> > threads;
	};

	static std::shared_ptr<Internals> GetInternals();

private:

	std::shared_ptr<Internals> m_int;
	static std::weak_ptr<Internals> s_int_inter_thread;
	static std::recursive_mutex	s_lock_int_inter_thread;
};

