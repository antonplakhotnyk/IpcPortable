#include "TestabilityInterfacePCH.h"
#include "IpcWin_WsaStartup.h"

// #pragma comment(lib, "Ws2_32.lib")


std::weak_ptr<IpcWin_WsaStartup> IpcWin_WsaStartup::s_wsa_startup;
std::mutex IpcWin_WsaStartup::s_mutex;

std::shared_ptr<IpcWin_WsaStartup> IpcWin_WsaStartup::Retain()
{
	std::lock_guard<std::mutex> lock(s_mutex);
	std::shared_ptr<IpcWin_WsaStartup> instance = s_wsa_startup.lock();
	if( !instance )
	{
		instance.reset(new IpcWin_WsaStartup);
		s_wsa_startup = instance;
	}
	return instance;
}

IpcWin_WsaStartup::IpcWin_WsaStartup()
{
	int wsa_startup_res = WSAStartup(MAKEWORD(2, 2), &m_wsa_data);
	return_if_not_equal(wsa_startup_res, 0);
	m_init_ok = true;
}

IpcWin_WsaStartup::~IpcWin_WsaStartup()
{
	if( m_init_ok )
		WSACleanup();
}
