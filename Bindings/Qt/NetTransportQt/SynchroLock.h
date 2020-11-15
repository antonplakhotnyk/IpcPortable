#pragma once

// Ёто тут инклюдить не об€зательно.
// ѕроинклюжено дл€ удобства использовани€ SynchroLock
#include "SynchroLockSection.h"
#include <QtCore/QMutex>

class SynchroLock: public ISynchroLock
{
public:
	SynchroLock();
	~SynchroLock();

	virtual void Enter();
	virtual void Leave();
private:
	QMutex	m_criticalsection;
};
