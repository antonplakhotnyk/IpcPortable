#pragma once

#include "MAssIpcCall.h"
#include "IpcClientTcpTransport.h"

#include "PostInterThread.h"

class IpcClientNet: public QObject, private PostInterThread
{
public:

	Q_OBJECT;

	IpcClientNet(void);
	~IpcClientNet(void);

	MAssIpcCall& Call();

	void Init(const Handlers& handlers, const char* ip_address, uint16_t target_port);

	void WaitConnection();

	static bool IsExist();
	static MAssIpcCall& Get();

	void SendLog(const char* msg, int msg_char_count) override;

	void OnPostInterThread() override;

signals:
	void OnDisconnected();
	void OnConnected();

private:

	std::vector<std::string> m_thread_messages;
	SynchroLock	m_thread_messages_lock;
	size_t m_constructor_thread_id;

	std::shared_ptr<MAssIpcCall> m_ipc_call;
	std::unique_ptr<IpcClientTcpTransport> m_transport;

	static IpcClientNet* s_instance;
};

