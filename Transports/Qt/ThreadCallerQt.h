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

protected:



	class CallWaiter
	{
	public:

		CallWaiter(std::shared_ptr<QMutex> wait_call_sync)
		{
		};

		void WaitProcessing();
		void CallDone();

	private:
		bool m_call_done;
		QWaitCondition	m_call_done_condition;
		std::shared_ptr<QMutex> m_wait_call_sync;
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

		std::shared_ptr<QMutex> GetWaitCallSync();

	private:

		std::shared_ptr<QMutex> m_wait_call_sync = std::shared_ptr<QMutex>(new QMutex);
	};

protected:

	void CallFromThread(MAssIpcThreadTransportTarget::Id thread_id, CallEvent* call_take_ownership,
						std::shared_ptr<CallWaiter>* call_waiter);
	void CallNoThread(CallEvent* call_take_ownership);
	
	static void ProcessCalls();

private:




	struct Internals
	{
		QMutex	lock_thread_receivers;
		std::map<MAssIpcThreadTransportTarget::Id, std::unique_ptr<ThreadCallReceiver> > thread_receivers;
	};

	std::shared_ptr<Internals> m_int;
	static std::weak_ptr<Internals> s_int_inter_thread;
	static QMutex	s_lock_int_inter_thread;

	CallReceiver	m_receiver_no_thread;
};
