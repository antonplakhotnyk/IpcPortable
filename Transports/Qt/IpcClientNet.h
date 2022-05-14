#pragma once

#include "MAssIpcCall.h"
#include "IpcClientTcpTransport.h"
#include "IpcNet.h"
#include <mutex>

class IpcClientNet: public QObject
{
public:

	IpcClientNet();
	~IpcClientNet();

	MAssIpcCall& Ipc();

	void WaitConnection();

// 	static bool IsExist();
// 	static MAssIpcCall& Get();

	IpcClientTcpTransport& GetIpcClientTcpTransport();

private:

// 	std::vector<std::string> m_thread_messages;
// 	std::mutex	m_thread_messages_lock;

	std::shared_ptr<IpcClientTcpTransport> m_transport;
	IpcNet	m_ipc_net;

//	static IpcClientNet* s_instance;
};

