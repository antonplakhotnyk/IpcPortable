#include "IpcQt_Transthread.h"
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>

IpcQt_Transthread::IpcQt_Transthread()
{
	AddTargetThread(QThread::currentThread());
}

IpcQt_Transthread::~IpcQt_Transthread()
{
}


void IpcQt_Transthread::CallFromThread(MAssIpc_TransthreadTarget::Id thread_id, std::unique_ptr<Job> job)
{
	std::unique_ptr<JobEvent> call(std::make_unique<JobEvent>(std::move(job)));
	IpcQt_TransthreadCaller::CallFromThread(thread_id, std::move(call), nullptr);
}

MAssIpc_TransthreadTarget::Id	IpcQt_Transthread::GetResultSendThreadId()
{
	return MAssIpc_TransthreadTarget::Id();
}

//-------------------------------------------------------

void IpcQt_Transthread::JobEvent::CallFromTargetThread()
{
	m_job->Invoke();
}
