#pragma once

#include <QtCore/QThread>


// This class only shows interface required by library.
// you responsible for implementing this interface according to your needs.
//
// Thread transport id may be implemented not same as std::threads::id and may be not same as MAssIpc_ThreadSafe::Id
class MAssIpc_TransthreadTarget
{
public:

	using Id = QThread*;

	static Id CurrentThread()
	{
		return QThread::currentThread();
	}

	static Id DirectCallPseudoId()
	{
		return Id();
	}

};
