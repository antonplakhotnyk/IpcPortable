#pragma once

#include <QtCore/QThread>
#include <QtCore/QEvent>
#include <QtCore/QCoreApplication>

#include "MAssIpc_TransthreadCaller.h"
#include "MAssIpc_Transthread.h"
#include "IpcQtBind_TransthreadCaller.h"
#include "MAssIpc_Macros.h"


class IpcQt_TransthreadCaller: public MAssIpc_TransthreadCaller, public MAssIpc_Transthread, public IpcQtBind_TransthreadCaller
{
public:

	IpcQt_TransthreadCaller()
	{
		CreateInternals<InternalsImpl>();
		AddTargetThread(QThread::currentThread());
	}

	~IpcQt_TransthreadCaller()
	{
	}

	static MAssIpc_TransthreadTarget::Id	AddTargetThread(QThread* sender_or_receiver)
	{
		MAssIpc_TransthreadTarget::Id thread_id = IpcQtBind_TransthreadCaller::GetId(sender_or_receiver);
		return MAssIpc_TransthreadCaller::AddTargetThreadId(thread_id);
	}

	struct CallEvent: QObject, QEvent, MAssIpc_TransthreadCaller::Call
	{
		CallEvent(): QEvent(QEvent::User){}
	};

	std::shared_ptr<MAssIpc_TransthreadCaller::CallWaiter>			CallFromThread(MAssIpc_TransthreadTarget::Id thread_id, std::unique_ptr<CallEvent> call)
	{
		return MAssIpc_TransthreadCaller::CallFromThread(thread_id, std::move(call));
	}

private:

	void			CallFromThread(MAssIpc_TransthreadTarget::Id thread_id, std::unique_ptr<Job> job) override
	{
		std::unique_ptr<JobEvent> call(std::make_unique<JobEvent>(std::move(job)));
		MAssIpc_TransthreadCaller::CallFromThread(thread_id, std::move(call));
	}

	MAssIpc_TransthreadTarget::Id	GetResultSendThreadId() override
	{
		return MAssIpc_TransthreadTarget::DirectCallPseudoId();
	}



	struct InternalsImpl: MAssIpc_TransthreadCaller::Internals
	{
		void PostEvent(const std::shared_ptr<ThreadCallReceiver>& receiver, std::unique_ptr<Call> call) override
		{
			std::unique_ptr<CallEvent> call_qevent{dynamic_cast_unique_ptr<CallEvent>(std::move(call))};
			std::shared_ptr<Receiver> reciver_qobject = std::dynamic_pointer_cast<Receiver>(receiver);
			return_if_equal_msg_mass_ipc(bool(call_qevent), false, assert_msg_unexpected);
			return_if_equal_msg_mass_ipc(bool(reciver_qobject), false, assert_msg_unexpected);
			QCoreApplication::postEvent(reciver_qobject.get(), call_qevent.release());
		}

		void ProcessReceiver(const std::shared_ptr<ThreadCallReceiver>& receiver) override
		{
			std::shared_ptr<Receiver> reciver_qobject = std::dynamic_pointer_cast<Receiver>(receiver);
			return_if_equal_msg_mass_ipc(bool(reciver_qobject), false, assert_msg_unexpected);
			QCoreApplication::sendPostedEvents(reciver_qobject.get());
		}

		std::shared_ptr<ThreadCallReceiver> CreateReceiver(MAssIpc_TransthreadTarget::Id sender_or_receiver_id) override
		{
			QThread* thread_sender_or_receiver = IpcQtBind_TransthreadCaller::GetQThread(sender_or_receiver_id);
			std::shared_ptr<Receiver> sender_or_receiver = std::make_shared<Receiver>();
			QObject::connect(thread_sender_or_receiver, &QThread::finished, sender_or_receiver.get(), &Receiver::OnFinished_ReceiverThread);
			sender_or_receiver->moveToThread(thread_sender_or_receiver);
			return sender_or_receiver;
		}
	};

private:

	struct Receiver:QObject, ThreadCallReceiver
	{
		void OnFinished_ReceiverThread()
		{
			ThreadCallReceiver::OnFinished_ReceiverThread();
		}


		void customEvent(QEvent* event) override
		{
			CallEvent* ev = dynamic_cast<CallEvent*>(event);
			return_if_equal_msg_mass_ipc(ev, nullptr, assert_msg_unexpected);
			ev->ProcessCallFromTargetThread();
		}
	};

	struct JobEvent: CallEvent
	{
		JobEvent(std::unique_ptr<MAssIpc_Transthread::Job> job)
			: m_job(std::move(job))
		{
		}
	private:
		void CallFromTargetThread() override
		{
			m_job->Invoke();
		}
	private:
		std::unique_ptr<MAssIpc_Transthread::Job> m_job;
	};


};
