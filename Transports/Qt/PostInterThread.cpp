#include "StdAfx.h"
#include "PostInterThread.h"
#include <QtCore/QEvent>
#include <QtCore/QObject>
#include <QtCore/QCoreApplication>

struct PostInterThread_Imp: public QObject
{
	PostInterThread_Imp(PostInterThread* inter_thread)
		:m_inter_thread(inter_thread)
	{
	}

	~PostInterThread_Imp()
	{
	}

	enum EventType {et_simple=QEvent::User, et_with_id=QEvent::User+1};

	class IdParamEvent: public QEvent
	{
	public:

		IdParamEvent(int param):QEvent(QEvent::Type(PostInterThread_Imp::et_with_id)),m_param(param){};

		int m_param;
	};

	virtual void customEvent(QEvent* ev)
	{
		if(m_inter_thread==NULL)
			return;

		if(ev->type()==et_simple)
			m_inter_thread->OnPostInterThread();
		else if(ev->type()==et_with_id)
		{
			IdParamEvent* ce=static_cast<IdParamEvent*>(ev);
			m_inter_thread->OnPostInterThreadId(ce->m_param);
		}
	}


	PostInterThread* m_inter_thread;
};

PostInterThread::PostInterThread()
{
	m_imp = new PostInterThread_Imp(this);
}

PostInterThread::~PostInterThread()
{
	m_imp->m_inter_thread=NULL;
	delete m_imp;
}

void PostInterThread::PostEvent()
{
	QEvent* ev=new QEvent(QEvent::Type(PostInterThread_Imp::et_simple));
	QCoreApplication::postEvent(m_imp, ev);
}

void PostInterThread::PostEventId(int id)
{
	QEvent* ev=new PostInterThread_Imp::IdParamEvent(id);
	QCoreApplication::postEvent(m_imp, ev);
}
