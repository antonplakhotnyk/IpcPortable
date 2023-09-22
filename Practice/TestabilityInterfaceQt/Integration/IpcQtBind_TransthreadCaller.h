#pragma once

#include "MAssIpc_TransthreadTarget.h"
#include <QtCore/QThread>

class IpcQtBind_TransthreadCaller
{
public:

	static MAssIpc_TransthreadTarget::Id GetId(QThread* thread)
	{
		return thread;
	}

	static QThread* GetQThread(MAssIpc_TransthreadTarget::Id id)
	{
		return id;
	}

};