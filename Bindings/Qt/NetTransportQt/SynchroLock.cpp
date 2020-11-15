#include "StdAfx.h"
#include "SynchroLock.h"

SynchroLock::SynchroLock()
{
}

SynchroLock::~SynchroLock()
{
}

void SynchroLock::Enter()
{
	m_criticalsection.lock();
}

void SynchroLock::Leave()
{
	m_criticalsection.unlock();
}
