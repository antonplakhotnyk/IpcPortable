#pragma once

#include "DelegateWCaller_Simple.h"
#include <QtCore/QObject>

class DelegateWCaller_Window: public DelegateWCaller_Simple, public QObject
{
public:

	DelegateWCaller_Window(void);
	~DelegateWCaller_Window(void);

private:


	virtual void customEvent(QEvent* ev);

	void PostRequest() override;
	void MarkRequestProcessed() override;


private:

	bool				m_request_posted;
};
