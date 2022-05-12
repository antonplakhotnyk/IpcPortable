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
	std::unique_ptr<JobEvent> call(std::make_unique<JobEvent>(std::move(job)));
	ThreadCallerQt::CallFromThread(thread_id, std::move(call), nullptr);
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
