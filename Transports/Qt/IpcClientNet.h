#pragma once

#include "MAssIpcCall.h"
#include "IpcClientTcpTransport.h"
#include "PostInterThread.h"
#include "IpcThreadTransportQt.h"
#include <mutex>

class IpcClientNet: public QObject
{
public:

	IpcClientNet();
	~IpcClientNet();

	MAssIpcCall& Ipc();

	void Init(const char* remote_address, uint16_t target_port);

	void WaitConnection();

// 	static bool IsExist();
// 	static MAssIpcCall& Get();

	IpcTcpTransportQt& GetTransportQt();

private:

	std::vector<std::string> m_thread_messages;
	std::mutex	m_thread_messages_lock;
	const MAssIpcThreadTransportTarget::Id m_constructor_thread_id;

	std::unique_ptr<MAssIpcCall> m_ipc_call;
	std::shared_ptr<IpcClientTcpTransport> m_transport;
	std::shared_ptr<IpcThreadTransportQt> m_thread_transport;
	

//	static IpcClientNet* s_instance;
};

