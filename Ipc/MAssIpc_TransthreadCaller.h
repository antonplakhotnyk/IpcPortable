#pragma once

#include "MAssIpc_TransthreadTarget.h"
#include "MAssIpc_ThreadSafe.h"
#include <memory>
#include <mutex>
#include <condition_variable>
#include <set>
#include <map>

class MAssIpc_TransthreadCaller
{
	class CallWaiterPrivate;

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

	class Call
	{
	public:

		virtual ~Call() = default;

		void ProcessCallFromTargetThread();

		void SetCallWaiter(std::shared_ptr<CallWaiterPrivate> call_waiter);

	protected:
		virtual void CallFromTargetThread() = 0;

	protected:

		std::shared_ptr<CallWaiterPrivate> m_call_waiter;
	};



	static void CancelDisableWaitingCall(MAssIpc_TransthreadTarget::Id thread_waiting_call);

	static MAssIpc_TransthreadTarget::Id	AddTargetThreadId(MAssIpc_TransthreadTarget::Id sender_or_receiver_id);
	std::shared_ptr<CallWaiter>		CallFromThread(MAssIpc_TransthreadTarget::Id receiver_thread_id, std::unique_ptr<Call> call);


protected:

	MAssIpc_TransthreadCaller();
	~MAssIpc_TransthreadCaller();
	template<class TInternals>
	void CreateInternals()
	{
		std::unique_lock<std::recursive_mutex> lock(s_lock_int_inter_thread);
		m_int = s_int_inter_thread.lock();
		if( !m_int )
		{
			m_int = std::shared_ptr<Internals>(new TInternals);
			s_int_inter_thread = m_int;
		}
	}

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



protected:


	struct ThreadCallReceiver
	{
		virtual ~ThreadCallReceiver() = default;
		void OnFinished_ReceiverThread();

		std::shared_ptr<WaitSync> GetWaitCallSync() const;

		std::shared_ptr<std::set<CallWaiterPrivate*> > m_call_waiters_sender = std::make_shared<std::set<CallWaiterPrivate*>>();
		std::shared_ptr<std::set<CallWaiterPrivate*> > m_call_waiters_receiver = std::make_shared<std::set<CallWaiterPrivate*>>();

	private:
		const std::shared_ptr<WaitSync> m_sender_wait_return_receiver_processing_calls = std::make_shared<WaitSync>();
	};

	struct Internals
	{
		template <typename Target, typename Source>
		std::unique_ptr<Target> dynamic_cast_unique_ptr(std::unique_ptr<Source>&& source)
		{
			if( auto* casted_ptr = dynamic_cast<Target*>(source.get()) )
				return source.release(), std::unique_ptr<Target>(casted_ptr);
			return nullptr;
		}


		virtual std::shared_ptr<ThreadCallReceiver> CreateReceiver(MAssIpc_TransthreadTarget::Id sender_or_receiver_id) = 0;
		virtual void PostEvent(const std::shared_ptr<ThreadCallReceiver>& receiver, std::unique_ptr<Call> call) = 0;
		virtual void ProcessReceiver(const std::shared_ptr<ThreadCallReceiver>& receiver) = 0;


		const std::shared_ptr<std::recursive_mutex>	lock_threads = std::make_shared<std::recursive_mutex>();
		std::map<MAssIpc_TransthreadTarget::Id, std::shared_ptr<ThreadCallReceiver> > threads;
	};

	static std::shared_ptr<Internals> GetInternals();
	decltype(Internals::threads)::iterator MakeFindThread(MAssIpc_TransthreadTarget::Id sender_or_receiver_id);

protected:
	std::shared_ptr<Internals> m_int;// derived implementation class must indirectly initialize it by calling CreateInternals()
private:
	static std::weak_ptr<Internals> s_int_inter_thread;
	static std::recursive_mutex	s_lock_int_inter_thread;
};