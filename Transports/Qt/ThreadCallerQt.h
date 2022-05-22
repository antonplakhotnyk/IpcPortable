#pragma once

#include <QtCore/QThread>
#include <QtCore/QEvent>
#include <memory>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include "MAssIpcThreadTransportTarget.h"
#include <set>

class ThreadCallerQt: public QObject
{
public:
	ThreadCallerQt();
	~ThreadCallerQt();

	static MAssIpcThreadTransportTarget::Id	AddTargetThread(QThread* sender_or_receiver);

	static MAssIpcThreadTransportTarget::Id	GetCurrentThreadId();
	static MAssIpcThreadTransportTarget::Id	GetId(QThread* thread);
	static void CancelDisableWaitingCall(MAssIpcThreadTransportTarget::Id thread_waiting_call);

public:

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

	void CallFromThread(MAssIpcThreadTransportTarget::Id receiver_thread_id, std::unique_ptr<CallEvent> call,
						std::unique_ptr<CancelHolder>* call_waiter);
protected:
	void CallNoThread(std::unique_ptr<CallEvent> call);
	
	static void ProcessCalls();

private:

	struct Internals
	{
		std::mutex	lock_threads;
		std::map<MAssIpcThreadTransportTarget::Id, std::unique_ptr<ThreadCallReceiver> > threads;
	};

	std::shared_ptr<Internals> m_int;
	static std::weak_ptr<Internals> s_int_inter_thread;
	static std::mutex	s_lock_int_inter_thread;

	CallReceiver	m_receiver_no_thread;
};

