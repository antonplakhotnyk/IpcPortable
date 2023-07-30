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

private:

	struct WaitSync
	{
		bool receiver_thread_finished = false;
		std::condition_variable	condition;
		std::mutex mutex_sync;

		void SetReceiverThreadFinished();
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
		void CallCancel();

	private:
		enum CallState: uint8_t
		{
			cs_inprogress,
			cs_canceled,
			cs_done,
		};
		CallState m_call_state = cs_inprogress;
		std::shared_ptr<WaitSync> m_wait_return_processing_calls;
	};

public:

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

	struct CancelHolder
	{
		CancelHolder(std::shared_ptr<std::set<CancelHolder*> > sender_waiting_calls, const std::shared_ptr<CallWaiter>& call_waiter)
			: m_call_waiter_cancel(call_waiter)
			, m_sender_waiting_calls(sender_waiting_calls)
		{
			if( m_sender_waiting_calls )
				m_sender_waiting_calls->insert(this);
		}

		~CancelHolder()
		{
			if( m_sender_waiting_calls )
				m_sender_waiting_calls->erase(this);
		}

		std::shared_ptr<CallWaiter> m_call_waiter_cancel;

	private:

		std::shared_ptr<std::set<CancelHolder*> > m_sender_waiting_calls;
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

		std::shared_ptr<std::set<CancelHolder*> > m_sender_waiting_calls = std::make_shared<std::set<CancelHolder*>>();

	private:
		const std::shared_ptr<WaitSync> m_sender_wait_return_receiver_processing_calls = std::make_shared<WaitSync>();
	};

public:

	void CallFromThread(MAssIpc_TransthreadTarget::Id receiver_thread_id, std::unique_ptr<CallEvent> call,
						std::unique_ptr<CancelHolder>* call_waiter);
protected:
	void CallNoThread(std::unique_ptr<CallEvent> call);
	
	static void ProcessCalls();

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

