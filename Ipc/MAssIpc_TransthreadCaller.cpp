#include "MAssIpc_TransthreadCaller.h"
#include "MAssIpc_Macros.h"


std::weak_ptr<MAssIpc_TransthreadCaller::Internals> MAssIpc_TransthreadCaller::s_int_inter_thread;
std::recursive_mutex	MAssIpc_TransthreadCaller::s_lock_int_inter_thread;

MAssIpc_TransthreadCaller::MAssIpc_TransthreadCaller()
{
	m_int = GetInternals();
}

MAssIpc_TransthreadCaller::~MAssIpc_TransthreadCaller()
{
}

std::shared_ptr<MAssIpc_TransthreadCaller::Internals> MAssIpc_TransthreadCaller::GetInternals()
{
	std::unique_lock<std::recursive_mutex> lock(s_lock_int_inter_thread);
	return s_int_inter_thread.lock();
}

void MAssIpc_TransthreadCaller::CancelDisableWaitingCall(MAssIpc_TransthreadTarget::Id thread_waiting_call)
{
	std::shared_ptr<Internals> int_inter_thread = GetInternals();
	return_if_equal_mass_ipc(bool(int_inter_thread), false);

	{
		std::unique_lock<std::recursive_mutex> lock(*int_inter_thread->lock_threads.get());

		auto it_thread_waiting_call = int_inter_thread->threads.find(thread_waiting_call);
		return_if_equal_mass_ipc(it_thread_waiting_call, int_inter_thread->threads.end());

		std::shared_ptr<ThreadCallReceiver> thread_canceled = it_thread_waiting_call->second;
		if( bool(thread_canceled->m_call_waiters_sender) )
		{
			for( CallWaiterPrivate* call_waiter_cancel : *thread_canceled->m_call_waiters_sender.get() )
				call_waiter_cancel->CallCancel();

			thread_canceled->m_call_waiters_sender.reset();
		}
	}
}

decltype(MAssIpc_TransthreadCaller::Internals::threads)::iterator MAssIpc_TransthreadCaller::MakeFindThread(MAssIpc_TransthreadTarget::Id sender_or_receiver_id)
{
	auto it_receiver = m_int->threads.find(sender_or_receiver_id);
	if( it_receiver==m_int->threads.end() )
	{
		AddTargetThreadId(sender_or_receiver_id);
		it_receiver = m_int->threads.find(sender_or_receiver_id);
	}

	return it_receiver;
}

MAssIpc_TransthreadTarget::Id MAssIpc_TransthreadCaller::AddTargetThreadId(MAssIpc_TransthreadTarget::Id sender_or_receiver_id)
{
	std::shared_ptr<Internals> int_inter_thread = GetInternals();
	// instance of MAssIpc_TransthreadCaller - does not exist
	return_x_if_equal_mass_ipc(bool(int_inter_thread), false, MAssIpc_TransthreadTarget::CurrentThread());

	bool is_receiver_absent;
	{
		std::unique_lock<std::recursive_mutex> lock(*int_inter_thread->lock_threads.get());
		is_receiver_absent = (int_inter_thread->threads.find(sender_or_receiver_id)==int_inter_thread->threads.end());
	}

	if( is_receiver_absent )
	{
		std::shared_ptr<ThreadCallReceiver> thread_receiver = int_inter_thread->CreateReceiver(sender_or_receiver_id);

		{
			std::unique_lock<std::recursive_mutex> lock(*int_inter_thread->lock_threads.get());
			int_inter_thread->threads[sender_or_receiver_id] = std::move(thread_receiver);
		}
	}

	return sender_or_receiver_id;
}

std::shared_ptr<MAssIpc_TransthreadCaller::CallWaiter> MAssIpc_TransthreadCaller::CallFromThread(MAssIpc_TransthreadTarget::Id receiver_thread_id, std::unique_ptr<Call> call)
{
	//	qDebug()<<__func__<<' '<<QThread::currentThreadId()<<" ";
	if( receiver_thread_id == MAssIpc_TransthreadTarget::DirectCallPseudoId() )
	{
		call->ProcessCallFromTargetThread();
		return std::make_shared<CallWaiter>();
	}
	else
	{
		MAssIpc_TransthreadTarget::Id sender_thread_id = MAssIpc_TransthreadTarget::CurrentThread();

		std::shared_ptr<CallWaiterPrivate> call_waiter_new;
		std::shared_ptr<WaitSync> receiver_wait_return_processing_calls;
		{
			std::unique_lock<std::recursive_mutex> lock(*m_int->lock_threads.get());

			auto it_receiver = MakeFindThread(receiver_thread_id);
			return_x_if_equal_mass_ipc(it_receiver, m_int->threads.end(), std::make_shared<CallWaiter>());

			receiver_wait_return_processing_calls = it_receiver->second->GetWaitCallSync();

			auto it_sender = MakeFindThread(sender_thread_id);
			return_x_if_equal_mass_ipc(it_sender, m_int->threads.end(), std::make_shared<CallWaiter>());

			call_waiter_new = std::make_shared<CallWaiterPrivate>(it_sender->second->m_call_waiters_sender,
																  it_receiver->second->m_call_waiters_receiver,
																  it_sender->second->GetWaitCallSync(),
																  m_int->lock_threads);
			call->SetCallWaiter(call_waiter_new);
			if( !bool(it_sender->second->m_call_waiters_sender) )
				call_waiter_new->CallCancel();

			m_int->PostEvent(it_receiver->second, std::move(call));
		}
		if( receiver_wait_return_processing_calls )
			receiver_wait_return_processing_calls->ProcessIncomingCall();

		return call_waiter_new;
	}
}

void MAssIpc_TransthreadCaller::ProcessCalls()
{
	std::shared_ptr<Internals> int_inter_thread = GetInternals();
	return_if_equal_mass_ipc(bool(int_inter_thread), false);

	//	qDebug()<<__func__<<' '<<QThread::currentThreadId()<<" ";

	MAssIpc_TransthreadTarget::Id thread_id = MAssIpc_TransthreadTarget::CurrentThread();

	std::shared_ptr<ThreadCallReceiver> thread_receiver;
	{
		std::unique_lock<std::recursive_mutex> lock(*int_inter_thread->lock_threads.get());

		auto it = int_inter_thread->threads.find(thread_id);
		return_if_equal_mass_ipc(it, int_inter_thread->threads.end());

		thread_receiver = it->second;
	}

	int_inter_thread->ProcessReceiver(thread_receiver);
}

//-------------------------------------------------------

void MAssIpc_TransthreadCaller::Call::ProcessCallFromTargetThread()
{
	CallFromTargetThread();
	if( m_call_waiter )
		m_call_waiter->CallDone();
}

void MAssIpc_TransthreadCaller::Call::SetCallWaiter(std::shared_ptr<CallWaiterPrivate> call_waiter)
{
	if( !m_call_waiter )
		m_call_waiter = call_waiter;
}

//-------------------------------------------------------

void MAssIpc_TransthreadCaller::ThreadCallReceiver::OnFinished_ReceiverThread()
{
	auto current_thread_id = MAssIpc_TransthreadTarget::CurrentThread();

	std::shared_ptr<Internals> int_inter_thread = GetInternals();
	if( !bool(int_inter_thread) )
		return;

	std::shared_ptr<ThreadCallReceiver> safe_delete_lifetime;
	{
		std::unique_lock<std::recursive_mutex> lock(*int_inter_thread->lock_threads.get());
		auto it = int_inter_thread->threads.find(current_thread_id);
		if( it != int_inter_thread->threads.end() )
		{
			safe_delete_lifetime = std::move(it->second);
			int_inter_thread->threads.erase(it);

			if( bool(safe_delete_lifetime->m_call_waiters_receiver) )
				for( CallWaiterPrivate* call_waiter_cancel : *safe_delete_lifetime->m_call_waiters_receiver.get() )
					call_waiter_cancel->SetReceiverThreadFinished();
		}
	}
}

std::shared_ptr<MAssIpc_TransthreadCaller::WaitSync> MAssIpc_TransthreadCaller::ThreadCallReceiver::GetWaitCallSync() const
{
	return m_sender_wait_return_receiver_processing_calls;
}

//-------------------------------------------------------
MAssIpc_TransthreadCaller::CallWaiterPrivate::CallState MAssIpc_TransthreadCaller::CallWaiterPrivate::WaitProcessing()
{
	while( true )
	{
		{
			std::unique_lock<std::recursive_mutex> lock(m_wait_return_processing_calls->mutex_sync);
			//			qDebug()<<__func__<<' '<<QThread::currentThreadId()<<" lock";

			if( m_call_state!=cs_inprogress )
				break;

			if( m_wait_return_processing_calls->process_incoming_call )
				m_wait_return_processing_calls->process_incoming_call = false;
			else
			{
				//				qDebug()<<__func__<<' '<<QThread::currentThreadId()<<" wait";
				m_wait_return_processing_calls->condition.wait(lock);
				if( m_call_state!=cs_inprogress )
					break;
			}
		}

		MAssIpc_TransthreadCaller::ProcessCalls();
	}

	return m_call_state;
}

void MAssIpc_TransthreadCaller::CallWaiterPrivate::CallDone()
{
	std::unique_lock<std::recursive_mutex> lock(m_wait_return_processing_calls->mutex_sync);
	//	qDebug()<<__func__<<' '<<QThread::currentThreadId()<<" lock";
	m_call_state = cs_done;
	m_wait_return_processing_calls->condition.notify_all();
}

void MAssIpc_TransthreadCaller::CallWaiterPrivate::CallCancel()
{
	std::unique_lock<std::recursive_mutex> lock(m_wait_return_processing_calls->mutex_sync);
	m_call_state = cs_canceled;
	m_wait_return_processing_calls->condition.notify_all();
}

void MAssIpc_TransthreadCaller::CallWaiterPrivate::SetReceiverThreadFinished()
{
	std::unique_lock<std::recursive_mutex> lock(m_wait_return_processing_calls->mutex_sync);
	m_call_state = cs_receiver_thread_finished;
	m_wait_return_processing_calls->condition.notify_all();
}

void MAssIpc_TransthreadCaller::WaitSync::ProcessIncomingCall()
{
	std::unique_lock<std::recursive_mutex> lock(this->mutex_sync);
	//	qDebug()<<__func__<<' '<<QThread::currentThreadId()<<" lock";
	this->process_incoming_call = true;
	this->condition.notify_all();
}
