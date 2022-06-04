#include "IpcQt_TransthreadCaller.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QMutexLocker>
#include "MAssIpc_Macros.h"

std::weak_ptr<IpcQt_TransthreadCaller::Internals> IpcQt_TransthreadCaller::s_int_inter_thread;
std::mutex	IpcQt_TransthreadCaller::s_lock_int_inter_thread;

IpcQt_TransthreadCaller::IpcQt_TransthreadCaller()
{
	std::shared_ptr<Internals> new_int = std::shared_ptr<Internals>(new Internals);

	{
		std::unique_lock<std::mutex> lock(s_lock_int_inter_thread);
		m_int = s_int_inter_thread.lock();
		if( !m_int )
		{
			s_int_inter_thread = new_int;
			m_int = new_int;
		}
	}
}

IpcQt_TransthreadCaller::~IpcQt_TransthreadCaller()
{
}

MAssIpc_TransthreadTarget::Id IpcQt_TransthreadCaller::GetCurrentThreadId()
{
	return QThread::currentThread();
}

MAssIpc_TransthreadTarget::Id	IpcQt_TransthreadCaller::GetId(QThread* thread)
{
	return thread;
}

void IpcQt_TransthreadCaller::CancelDisableWaitingCall(MAssIpc_TransthreadTarget::Id thread_waiting_call)
{
	std::shared_ptr<Internals> int_inter_thread;
	{
		std::unique_lock<std::mutex> lock(s_lock_int_inter_thread);
		int_inter_thread = s_int_inter_thread.lock();
	}
	mass_return_if_equal(bool(int_inter_thread),false);

	{
		std::unique_lock<std::mutex> lock(int_inter_thread->lock_threads);

		auto it_thread_waiting_call = int_inter_thread->threads.find(thread_waiting_call);
		mass_return_if_equal(it_thread_waiting_call, int_inter_thread->threads.end());

		for( CancelHolder* holder : *it_thread_waiting_call->second->m_sender_waiting_calls.get() )
			holder->m_call_waiter_cancel->CallCancel();

		it_thread_waiting_call->second->m_sender_waiting_calls.reset();
	}
}


MAssIpc_TransthreadTarget::Id IpcQt_TransthreadCaller::AddTargetThread(QThread* sender_or_receiver)
{
	std::shared_ptr<Internals> int_inter_thread;
	{
		std::unique_lock<std::mutex> lock(s_lock_int_inter_thread);
		int_inter_thread = s_int_inter_thread.lock();
	}
	// instance of IpcQt_TransthreadCaller - does not exist
	mass_return_x_if_equal(bool(int_inter_thread),false, MAssIpc_TransthreadTarget::Id());

	MAssIpc_TransthreadTarget::Id receiver_id = IpcQt_TransthreadCaller::GetId(sender_or_receiver);
	bool is_receiver_absent;
	{
		std::unique_lock<std::mutex> lock(int_inter_thread->lock_threads);
		is_receiver_absent = (int_inter_thread->threads.find(receiver_id)==int_inter_thread->threads.end());
	}

	if( is_receiver_absent )
	{
		std::unique_ptr<ThreadCallReceiver> thread_receiver(std::make_unique<ThreadCallReceiver>());
		QObject::connect(sender_or_receiver, &QThread::finished, thread_receiver.get(), &ThreadCallReceiver::OnFinished_ReceiverThread);

		{
			std::unique_lock<std::mutex> lock(int_inter_thread->lock_threads);
			thread_receiver->moveToThread(sender_or_receiver);
			int_inter_thread->threads[receiver_id] = std::move(thread_receiver);
		}
	}

	return receiver_id;
}

void IpcQt_TransthreadCaller::CallFromThread(MAssIpc_TransthreadTarget::Id receiver_thread_id, std::unique_ptr<CallEvent> call,
									std::unique_ptr<CancelHolder>* call_waiter)
{
	MAssIpc_TransthreadTarget::Id sender_thread_id = IpcQt_TransthreadCaller::GetCurrentThreadId();
	if( receiver_thread_id == MAssIpc_TransthreadTarget::Id() )
		receiver_thread_id = sender_thread_id;

	std::shared_ptr<WaitSync> wait_return_processing_calls;
	{
		std::unique_lock<std::mutex> lock(m_int->lock_threads);

		auto it_receiver = m_int->threads.find(receiver_thread_id);
		mass_return_if_equal(it_receiver, m_int->threads.end());
		wait_return_processing_calls = it_receiver->second->GetWaitCallSync();

		if( call_waiter )
		{
			auto it_sender = m_int->threads.find(sender_thread_id);
			mass_return_if_equal(it_sender, m_int->threads.end());

			std::shared_ptr<CallWaiter> call_waiter_new = std::make_shared<CallWaiter>(wait_return_processing_calls);
			call->SetCallWaiter(call_waiter_new);
			if( !bool(it_sender->second->m_sender_waiting_calls) )
				call_waiter_new->CallCancel();

			*call_waiter = std::make_unique<CancelHolder>(it_sender->second->m_sender_waiting_calls, call_waiter_new);
		}
		QCoreApplication::postEvent(it_receiver->second.get(), call.release());
	}
	if( wait_return_processing_calls )
		wait_return_processing_calls->condition.notify_all();
}

void IpcQt_TransthreadCaller::ProcessCalls()
{
	MAssIpc_TransthreadTarget::Id thread_id = IpcQt_TransthreadCaller::GetCurrentThreadId();
	std::shared_ptr<Internals> int_inter_thread;

	{
		std::unique_lock<std::mutex> lock(s_lock_int_inter_thread);
		int_inter_thread = s_int_inter_thread.lock();
	}

	ThreadCallReceiver* thread_receiver;
	{
		std::unique_lock<std::mutex> lock(int_inter_thread->lock_threads);

		auto it = int_inter_thread->threads.find(thread_id);
		mass_return_if_equal(it, int_inter_thread->threads.end());

		thread_receiver = it->second.get();
	}

	QCoreApplication::sendPostedEvents(thread_receiver);
}

void IpcQt_TransthreadCaller::CallNoThread(std::unique_ptr<CallEvent> call)
{
	QCoreApplication::postEvent(&m_receiver_no_thread, call.release());
}

//-------------------------------------------------------

void IpcQt_TransthreadCaller::CallReceiver::customEvent(QEvent* event)
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

void IpcQt_TransthreadCaller::CallEvent::SetCallWaiter(std::shared_ptr<CallWaiter> call_waiter)
{
	if( !m_call_waiter )
		m_call_waiter = call_waiter;
}

//-------------------------------------------------------

void IpcQt_TransthreadCaller::ThreadCallReceiver::OnFinished_ReceiverThread()
{
	auto current_thread_id = IpcQt_TransthreadCaller::GetCurrentThreadId();

	std::unique_ptr<ThreadCallReceiver> safe_delete_lifetime;

	{
		std::unique_lock<std::mutex> lock(s_lock_int_inter_thread);
		std::shared_ptr<Internals> int_inter_thread;

		int_inter_thread = s_int_inter_thread.lock();
		if( int_inter_thread )
		{
			std::unique_lock<std::mutex> lock(int_inter_thread->lock_threads);
			auto it = int_inter_thread->threads.find(current_thread_id);
			if( it != int_inter_thread->threads.end() )
			{
				safe_delete_lifetime = std::move(it->second);
				int_inter_thread->threads.erase(it);

				std::shared_ptr<WaitSync> wait_return_processing_calls = safe_delete_lifetime->GetWaitCallSync();
				wait_return_processing_calls->SetReceiverThreadFinished();
			}
		}
	}
}

std::shared_ptr<IpcQt_TransthreadCaller::WaitSync> IpcQt_TransthreadCaller::ThreadCallReceiver::GetWaitCallSync() const
{
	return m_sender_wait_return_receiver_processing_calls;
}

//-------------------------------------------------------
void IpcQt_TransthreadCaller::CallWaiter::WaitProcessing()
{
	while(true)
	{
		{
			std::unique_lock<std::mutex> lock(m_wait_return_processing_calls->mutex_sync);
			if( (m_call_state!=cs_inprogress) || m_wait_return_processing_calls->receiver_thread_finished )
				break;

			m_wait_return_processing_calls->condition.wait(lock);
			if( (m_call_state!=cs_inprogress) || m_wait_return_processing_calls->receiver_thread_finished )
				break;
		}

		IpcQt_TransthreadCaller::ProcessCalls();
	}
}

void IpcQt_TransthreadCaller::CallWaiter::CallDone()
{
	std::unique_lock<std::mutex> lock(m_wait_return_processing_calls->mutex_sync);
	m_call_state = cs_done;
	m_wait_return_processing_calls->condition.notify_all();
}

void IpcQt_TransthreadCaller::CallWaiter::CallCancel()
{
	std::unique_lock<std::mutex> lock(m_wait_return_processing_calls->mutex_sync);
	m_call_state = cs_canceled;
	m_wait_return_processing_calls->condition.notify_all();
}

void IpcQt_TransthreadCaller::WaitSync::SetReceiverThreadFinished()
{
	std::unique_lock<std::mutex> lock(this->mutex_sync);
	this->receiver_thread_finished = true;
	this->condition.notify_all();
}
