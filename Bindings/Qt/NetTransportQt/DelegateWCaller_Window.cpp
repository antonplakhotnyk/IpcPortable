#include "StdAfx.h"
#include "DelegateWCaller_Window.h"
#include <QtCore/QEvent>
#include <QtCore/QCoreApplication>

DelegateWCaller_Window::DelegateWCaller_Window(void)
: m_request_posted(false)
{
}

DelegateWCaller_Window::~DelegateWCaller_Window(void)
{
}

void DelegateWCaller_Window::customEvent(QEvent* ev)
{
	ProcessCalls();
}

void DelegateWCaller_Window::PostRequest()
{
	// PostIfNotInQueue не катит потому что она реализована через PeekMessage
	//		которая прокачивает сообщения а это недопустимо.
	//		PostIfNotInQueue(m_window.GetHandle(),WM_USER,NULL,NULL);
	if( !m_request_posted )
	{
		m_request_posted = true;
		QEvent* ev = new QEvent(QEvent::User);
		QCoreApplication::postEvent(this, ev);
	}
}

void DelegateWCaller_Window::MarkRequestProcessed()
{
	m_request_posted = false;
}