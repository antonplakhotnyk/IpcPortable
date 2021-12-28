#include "stdafx.h"
#include "IpcThreadTransportQt.h"
#include <QThread>
#include <QCoreApplication>
#include "InternalThread2.h"

IpcThreadTransportQt::IpcThreadTransportQt()
{
	AddTargetThread(QThread::currentThread());
}

IpcThreadTransportQt::~IpcThreadTransportQt()
{
}


void IpcThreadTransportQt::CallFromThread(AVThread::Id thread_id, Job* job)
{
	JobEvent* call_take_ownership = new JobEvent(job);
	ThreadCallerQt::CallFromThread(thread_id, call_take_ownership, nullptr);
}

AVThread::Id IpcThreadTransportQt::GetCurrentThreadId()
{
	return InternalThread2::GetCurrentThreadId();
}

//-------------------------------------------------------

void IpcThreadTransportQt::JobEvent::CallFromTargetThread()
{
	m_job->Invoke();
}
