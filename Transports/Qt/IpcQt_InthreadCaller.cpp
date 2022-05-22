#include "IpcQt_InthreadCaller.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QMutexLocker>
#include "MAssMacros.h"

std::weak_ptr<IpcQt_InthreadCaller::Internals> IpcQt_InthreadCaller::s_int_inter_thread;
std::mutex	IpcQt_InthreadCaller::s_lock_int_inter_thread;

IpcQt_InthreadCaller::IpcQt_InthreadCaller()
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

IpcQt_InthreadCaller::~IpcQt_InthreadCaller()
{
}

MAssIpcThreadTransportTarget::Id IpcQt_InthreadCaller::GetCurrentThreadId()
{
	return QThread::currentThread();
}

MAssIpcThreadTransportTarget::Id	IpcQt_InthreadCaller::GetId(QThread* thread)
{
	return thread;
}

MAssIpcThreadTransportTarget::Id IpcQt_InthreadCaller::AddTargetThread(QThread* receiver)
{
	std::shared_ptr<Internals> int_inter_thread;
	{
		std::unique_lock<std::mutex> lock(s_lock_int_inter_thread);
		int_inter_thread = s_int_inter_thread.lock();
	}
	// instance of IpcQt_InthreadCaller - does not exist
	mass_return_x_if_equal(bool(int_inter_thread),false, MAssIpcThreadTransportTarget::Id());

	MAssIpcThreadTransportTarget::Id receiver_id = IpcQt_InthreadCaller::GetId(receiver);
	bool is_receiver_absent;
	{
		std::unique_lock<std::mutex> lock(int_inter_thread->lock_thread_receivers);
		is_receiver_absent = (int_inter_thread->thread_receivers.find(receiver_id)==int_inter_thread->thread_receivers.end());
	}

	if( is_receiver_absent )
	{
		std::unique_ptr<ThreadCallReceiver> thread_receiver(std::make_unique<ThreadCallReceiver>());
		QObject::connect(receiver, &QThread::finished, thread_receiver.get(), &ThreadCallReceiver::OnFinished_ReceiverThread);

		{
			std::unique_lock<std::mutex> lock(int_inter_thread->lock_thread_receivers);
			thread_receiver->moveToThread(receiver);
			int_inter_thread->thread_receivers[receiver_id] = std::move(thread_receiver);
		}
	}

	return receiver_id;
}

void IpcQt_InthreadCaller::CallFromThread(MAssIpcThreadTransportTarget::Id thread_id, std::unique_ptr<CallEvent> call,
									std::shared_ptr<CallWaiter>* call_waiter)
{
	if( thread_id == MAssIpcThreadTransportTarget::Id() )
		thread_id = IpcQt_InthreadCaller::GetCurrentThreadId();

	std::shared_ptr<WaitSync> wait_return_processing_calls;
	{
		std::unique_lock<std::mutex> lock(m_int->lock_thread_receivers);

		auto it = m_int->thread_receivers.find(thread_id);
		mass_return_if_equal(it, m_int->thread_receivers.end());
		wait_return_processing_calls = it->second->GetWaitCallSync();

		if( call_waiter )
		{
			std::shared_ptr<CallWaiter> call_waiter_new = std::make_shared<CallWaiter>(wait_return_processing_calls);
			call->SetCallWaiter(call_waiter_new);
			*call_waiter = call_waiter_new;
		}
		QCoreApplication::postEvent(it->second.get(), call.release());
	}
	if( wait_return_processing_calls )
		wait_return_processing_calls->condition.notify_all();
}

void IpcQt_InthreadCaller::ProcessCalls()
{
	MAssIpcThreadTransportTarget::Id thread_id = IpcQt_InthreadCaller::GetCurrentThreadId();
	std::shared_ptr<Internals> int_inter_thread;

	{
		std::unique_lock<std::mutex> lock(s_lock_int_inter_thread);
		int_inter_thread = s_int_inter_thread.lock();
	}

	ThreadCallReceiver* thread_receiver;
	{
		std::unique_lock<std::mutex> lock(int_inter_thread->lock_thread_receivers);

		auto it = int_inter_thread->thread_receivers.find(thread_id);
		mass_return_if_equal(it, int_inter_thread->thread_receivers.end());

		thread_receiver = it->second.get();
	}

	QCoreApplication::sendPostedEvents(thread_receiver);
}

void IpcQt_InthreadCaller::CallNoThread(std::unique_ptr<CallEvent> call)
{
	QCoreApplication::postEvent(&m_receiver_no_thread, call.release());
}

//-------------------------------------------------------

void IpcQt_InthreadCaller::CallReceiver::customEvent(QEvent* event)
{
	CallEvent* ev = static_cast<CallEvent*>(event);
	ev->ProcessCallFromTargetThread();
}

//-------------------------------------------------------

void IpcQt_InthreadCaller::CallEvent::ProcessCallFromTargetThread()
{
	CallFromTargetThread();
	if( m_call_waiter )
		m_call_waiter->CallDone();
}

void IpcQt_InthreadCaller::CallEvent::SetCallWaiter(std::shared_ptr<CallWaiter> call_waiter)
{
	if( !m_call_waiter )
		m_call_waiter = call_waiter;
}

//-------------------------------------------------------

void IpcQt_InthreadCaller::ThreadCallReceiver::OnFinished_ReceiverThread()
{
	auto current_thread_id = IpcQt_InthreadCaller::GetCurrentThreadId();

	std::unique_ptr<ThreadCallReceiver> safe_delete_lifetime;

	{
		std::unique_lock<std::mutex> lock(s_lock_int_inter_thread);
		std::shared_ptr<Internals> int_inter_thread;

		int_inter_thread = s_int_inter_thread.lock();
		if( int_inter_thread )
		{
			std::unique_lock<std::mutex> lock(int_inter_thread->lock_thread_receivers);
			auto it = int_inter_thread->thread_receivers.find(current_thread_id);
			if( it != int_inter_thread->thread_receivers.end() )
			{
				safe_delete_lifetime = std::move(it->second);
				int_inter_thread->thread_receivers.erase(it);
			}
		}
	}
}

std::shared_ptr<IpcQt_InthreadCaller::WaitSync> IpcQt_InthreadCaller::ThreadCallReceiver::GetWaitCallSync() const
{
	return m_wait_return_processing_calls;
}

//-------------------------------------------------------
void IpcQt_InthreadCaller::CallWaiter::WaitProcessing()
{
	while(true)
	{
		{
			std::unique_lock<std::mutex> lock(m_wait_return_processing_calls->mutex_sync);
			if( m_call_done )
				break;

			m_wait_return_processing_calls->condition.wait(lock);
			if( m_call_done )
				break;
		}

		IpcQt_InthreadCaller::ProcessCalls();
	}
}

void IpcQt_InthreadCaller::CallWaiter::CallDone()
{
	std::unique_lock<std::mutex> lock(m_wait_return_processing_calls->mutex_sync);
	m_call_done = true;
	m_wait_return_processing_calls->condition.notify_all();
}
