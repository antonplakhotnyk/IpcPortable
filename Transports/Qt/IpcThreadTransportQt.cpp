#include "IpcThreadTransportQt.h"
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>

IpcThreadTransportQt::IpcThreadTransportQt()
{
	AddTargetThread(QThread::currentThread());
}

IpcThreadTransportQt::~IpcThreadTransportQt()
{
}


void IpcThreadTransportQt::CallFromThread(MAssIpcThreadTransportTarget::Id thread_id, std::unique_ptr<Job> job)
{
	JobEvent* call_take_ownership = new JobEvent(std::move(job));
	ThreadCallerQt::CallFromThread(thread_id, call_take_ownership, nullptr);
}

MAssIpcThreadTransportTarget::Id	IpcThreadTransportQt::GetResultSendThreadId()
{
	return MAssIpcThreadTransportTarget::Id();
}

//-------------------------------------------------------

void IpcThreadTransportQt::JobEvent::CallFromTargetThread()
{
	m_job->Invoke();
}
