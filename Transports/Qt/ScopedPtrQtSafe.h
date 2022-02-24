#pragma once

#include <QtCore/QPointer>
#include "MAssMacros.h"

// Used for controling scope-deleted QObject with parent
template<class TVal>
class ScopedPtrQtSafe
{
private:

	QPointer<TVal> m_val;

private:
	ScopedPtrQtSafe(const ScopedPtrQtSafe&);
public:

	ScopedPtrQtSafe()
	{
	}

	~ScopedPtrQtSafe()
	{
		Release();
	}

	ScopedPtrQtSafe& operator=(TVal* other)
	{
		m_val = other;
		return *this;
	}

	template<class Arg1>
	ScopedPtrQtSafe(Arg1 a1):
		m_val(a1)
	{
	}

	TVal* operator ->() const
	{
		mass_return_x_if_equal(m_val,NULL,NULL);
		return m_val;
	}

	TVal* GetP() const
	{
		return m_val;
	}

	void Release()
	{
		if( m_val )
		{
			QPointer<TVal> t(m_val);
			m_val = NULL;
			delete t;
		}
	}
};

