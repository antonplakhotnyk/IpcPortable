#pragma once

#include <winsock2.h>
#include <mutex>

class IpcWin_WsaStartup
{
public:
	static std::shared_ptr<IpcWin_WsaStartup> Retain();

private:
	IpcWin_WsaStartup();

public:

	~IpcWin_WsaStartup();

private:

	bool m_init_ok = false;
	WSADATA m_wsa_data = {};
	static std::weak_ptr<IpcWin_WsaStartup> s_wsa_startup;
	static std::mutex s_mutex;
};
