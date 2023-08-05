#include "IpcQt_TransthreadCaller.h"
#include <QtCore/QCoreApplication>
#include "MAssIpc_Macros.h"

std::weak_ptr<IpcQt_TransthreadCaller::Internals> IpcQt_TransthreadCaller::s_int_inter_thread;
std::recursive_mutex	IpcQt_TransthreadCaller::s_lock_int_inter_thread;

IpcQt_TransthreadCaller::IpcQt_TransthreadCaller()
{
	std::unique_lock<std::recursive_mutex> lock(s_lock_int_inter_thread);
	m_int = s_int_inter_thread.lock();
	if( !m_int )
	{
		m_int = std::shared_ptr<Internals>(new Internals);
		s_int_inter_thread = m_int;
	}
}

IpcQt_TransthreadCaller::~IpcQt_TransthreadCaller()
{
}

std::shared_ptr<IpcQt_TransthreadCaller::Internals> IpcQt_TransthreadCaller::GetInternals()
{
	std::unique_lock<std::recursive_mutex> lock(s_lock_int_inter_thread);
	return s_int_inter_thread.lock();
}

void IpcQt_TransthreadCaller::CancelDisableWaitingCall(MAssIpc_TransthreadTarget::Id thread_waiting_call)
{
	std::shared_ptr<Internals> int_inter_thread = GetInternals();
	mass_return_if_equal(bool(int_inter_thread),false);

	{
		std::unique_lock<std::recursive_mutex> lock(*int_inter_thread->lock_threads.get());

		auto it_thread_waiting_call = int_inter_thread->threads.find(thread_waiting_call);
		mass_return_if_equal(it_thread_waiting_call, int_inter_thread->threads.end());

		std::shared_ptr<ThreadCallReceiver> thread_canceled = it_thread_waiting_call->second;
		if( bool(thread_canceled->m_call_waiters_sender) )
		{
			for( CallWaiterPrivate* call_waiter_cancel : *thread_canceled->m_call_waiters_sender.get() )
				call_waiter_cancel->CallCancel();

			thread_canceled->m_call_waiters_sender.reset();
		}
	}
}

MAssIpc_TransthreadTarget::Id IpcQt_TransthreadCaller::AddTargetThread(QThread* sender_or_receiver)
{
	std::shared_ptr<Internals> int_inter_thread = GetInternals();
	// instance of IpcQt_TransthreadCaller - does not exist
	mass_return_x_if_equal(bool(int_inter_thread),false, MAssIpc_TransthreadTarget::Id());

	MAssIpc_TransthreadTarget::Id receiver_id = IpcQt_TransthreadCaller::GetId(sender_or_receiver);
	bool is_receiver_absent;
	{
		std::unique_lock<std::recursive_mutex> lock(*int_inter_thread->lock_threads.get());
		is_receiver_absent = (int_inter_thread->threads.find(receiver_id)==int_inter_thread->threads.end());
	}

	if( is_receiver_absent )
	{
		std::unique_ptr<ThreadCallReceiver> thread_receiver(std::make_unique<ThreadCallReceiver>());
		QObject::connect(sender_or_receiver, &QThread::finished, thread_receiver.get(), &ThreadCallReceiver::OnFinished_ReceiverThread);
		thread_receiver->moveToThread(sender_or_receiver);

		{
			std::unique_lock<std::recursive_mutex> lock(*int_inter_thread->lock_threads.get());
			int_inter_thread->threads[receiver_id] = std::move(thread_receiver);
		}
	}

	return receiver_id;
}

std::shared_ptr<IpcQt_TransthreadCaller::CallWaiter> IpcQt_TransthreadCaller::CallFromThread(MAssIpc_TransthreadTarget::Id receiver_thread_id, std::unique_ptr<CallEvent> call)
{
	MAssIpc_TransthreadTarget::Id sender_thread_id = IpcQt_TransthreadCaller::GetCurrentThreadId();
	if( receiver_thread_id == MAssIpc_TransthreadTarget::Id() )
		receiver_thread_id = sender_thread_id;

	std::shared_ptr<CallWaiterPrivate> call_waiter_new;
	std::shared_ptr<WaitSync> receiver_wait_return_processing_calls;
	{
		std::unique_lock<std::recursive_mutex> lock(*m_int->lock_threads.get());

		auto it_receiver = m_int->threads.find(receiver_thread_id);
		mass_return_x_if_equal(it_receiver, m_int->threads.end(), std::make_shared<CallWaiter>());

		receiver_wait_return_processing_calls = it_receiver->second->GetWaitCallSync();

		auto it_sender = m_int->threads.find(sender_thread_id);
		mass_return_x_if_equal(it_sender, m_int->threads.end(), std::make_shared<CallWaiter>());

		call_waiter_new = std::make_shared<CallWaiterPrivate>(it_sender->second->m_call_waiters_sender, 
															  it_receiver->second->m_call_waiters_receiver, 
															  it_sender->second->GetWaitCallSync(),
															  m_int->lock_threads);
		call->SetCallWaiter(call_waiter_new);
		if( !bool(it_sender->second->m_call_waiters_sender) )
			call_waiter_new->CallCancel();

		QCoreApplication::postEvent(it_receiver->second.get(), call.release());
	}
	if( receiver_wait_return_processing_calls )
		receiver_wait_return_processing_calls->ProcessIncomingCall();

	return call_waiter_new;
}

void IpcQt_TransthreadCaller::ProcessCalls()
{
	std::shared_ptr<Internals> int_inter_thread = GetInternals();
	mass_return_if_equal(bool(int_inter_thread), false);

	MAssIpc_TransthreadTarget::Id thread_id = IpcQt_TransthreadCaller::GetCurrentThreadId();

	ThreadCallReceiver* thread_receiver;
	{
		std::unique_lock<std::recursive_mutex> lock(*int_inter_thread->lock_threads.get());

		auto it = int_inter_thread->threads.find(thread_id);
		mass_return_if_equal(it, int_inter_thread->threads.end());

		thread_receiver = it->second.get();
	}

	QCoreApplication::sendPostedEvents(thread_receiver);
}

//-------------------------------------------------------
void IpcQt_TransthreadCaller::ThreadCallReceiver::customEvent(QEvent* event)
{
	CallEvent* ev = static_cast<CallEvent*>(event);
	ev->ProcessCallFromTargetThread();
}

//-------------------------------------------------------

void IpcQt_TransthreadCaller::CallEvent::ProcessCallFromTargetThread()
{
	CallFromTargetThread();
	if( m_call_waiter )
		m_call_waiter->CallDone();
}

void IpcQt_TransthreadCaller::CallEvent::SetCallWaiter(std::shared_ptr<CallWaiterPrivate> call_waiter)
{
	if( !m_call_waiter )
		m_call_waiter = call_waiter;
}

//-------------------------------------------------------

void IpcQt_TransthreadCaller::ThreadCallReceiver::OnFinished_ReceiverThread()
{
	auto current_thread_id = IpcQt_TransthreadCaller::GetCurrentThreadId();

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

std::shared_ptr<IpcQt_TransthreadCaller::WaitSync> IpcQt_TransthreadCaller::ThreadCallReceiver::GetWaitCallSync() const
{
	return m_sender_wait_return_receiver_processing_calls;
}

//-------------------------------------------------------
IpcQt_TransthreadCaller::CallWaiterPrivate::CallState IpcQt_TransthreadCaller::CallWaiterPrivate::WaitProcessing()
{
	while(true)
	{
		{
			std::unique_lock<std::recursive_mutex> lock(m_wait_return_processing_calls->mutex_sync);
			if( m_call_state!=cs_inprogress )
				break;

			if( m_wait_return_processing_calls->process_incoming_call )
				m_wait_return_processing_calls->process_incoming_call = false;
			else
			{
				m_wait_return_processing_calls->condition.wait(lock);
				if( m_call_state!=cs_inprogress )
					break;
			}
		}

		IpcQt_TransthreadCaller::ProcessCalls();
	}

	return m_call_state;
}

void IpcQt_TransthreadCaller::CallWaiterPrivate::CallDone()
{
	std::unique_lock<std::recursive_mutex> lock(m_wait_return_processing_calls->mutex_sync);
	m_call_state = cs_done;
	m_wait_return_processing_calls->condition.notify_all();
}

void IpcQt_TransthreadCaller::CallWaiterPrivate::CallCancel()
{
	std::unique_lock<std::recursive_mutex> lock(m_wait_return_processing_calls->mutex_sync);
	m_call_state = cs_canceled;
	m_wait_return_processing_calls->condition.notify_all();
}

void IpcQt_TransthreadCaller::CallWaiterPrivate::SetReceiverThreadFinished()
{
	std::unique_lock<std::recursive_mutex> lock(m_wait_return_processing_calls->mutex_sync);
	m_call_state = cs_receiver_thread_finished;
	m_wait_return_processing_calls->condition.notify_all();
}

void IpcQt_TransthreadCaller::WaitSync::ProcessIncomingCall()
{
	std::unique_lock<std::recursive_mutex> lock(this->mutex_sync);
	this->process_incoming_call = true;
	this->condition.notify_all();
}
