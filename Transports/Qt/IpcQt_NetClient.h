#pragma once

#include "MAssIpcCall.h"
#include "IpcQt_TransportTcpClient.h"
#include "IpcQt_Net.h"
#include <mutex>

class IpcQt_NetClient: public QObject
{
public:

	IpcQt_NetClient(MAssIpcCall& ipc_connection);
	~IpcQt_NetClient();

	void WaitConnection();

// 	static bool IsExist();
// 	static MAssIpcCall& Get();

	std::shared_ptr<IpcQt_TransportTcpClient> GetIpcClientTcpTransport();

private:

// 	std::vector<std::string> m_thread_messages;
// 	std::mutex	m_thread_messages_lock;

	std::shared_ptr<IpcQt_TransportTcpClient> m_transport_client;
	IpcQt_Net	m_ipc_net;

//	static IpcQt_NetClient* s_instance;
};

