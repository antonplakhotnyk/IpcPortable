#pragma once

#include "IpcCall.h"
#include "EventgateW.h"
#include "IpcClientTcpTransport.h"
#include "AVLogSimple.h"

#include "PostInterThread.h"
#include "SynchroLock.h"

class IpcClientNet: private AVLogSimple, private PostInterThread
{
public:

	struct Handlers
	{
		EventgateW<void()> OnDisconnected;
		EventgateW<void()> OnConnected;
	};

	IpcClientNet(void);
	~IpcClientNet(void);

	IpcCall& Call();

	void Init(const Handlers& handlers, const char* ip_address, uint16_t target_port);

	void WaitConnection();

	static bool IsExist();
	static IpcCall& Get();

	void SendLog(const char* msg, int msg_char_count) override;

	void OnPostInterThread() override;


private:

	std::vector<std::string> m_thread_messages;
	SynchroLock	m_thread_messages_lock;
	size_t m_constructor_thread_id;

	OPtr<SO<IpcCall> > m_ipc_call;
	OPtr<IpcClientTcpTransport> m_transport;

	static IpcClientNet* s_instance;
};

