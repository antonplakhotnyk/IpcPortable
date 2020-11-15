#include "stdafx.h"
#include "IpcClientNet.h"
#include "AVPlatformSpecific.h"

IpcClientNet* IpcClientNet::s_instance=NULL;

IpcClientNet::IpcClientNet(void)
{
	assert_if_not_equal(s_instance,NULL);
	s_instance = this;

	m_constructor_thread_id = AVGetCurrentThreadId();

	m_transport = new IpcClientTcpTransport;
	m_ipc_call = new SO<IpcCall>(m_transport);
}

IpcClientNet::~IpcClientNet(void)
{
	if( s_instance==this )
		s_instance = NULL;
}

void IpcClientNet::Init(const Handlers& handlers, const char* ip_address, uint16_t target_port)
{
	{
		IpcClientTcpTransport::Handlers transport_handlers;
		transport_handlers.OnDisconnected = handlers.OnDisconnected;
		transport_handlers.OnConnected = handlers.OnConnected;
		transport_handlers.ProcessTransport.BindE(&IpcCall::ProcessTransport, m_ipc_call);
		m_transport->Init(transport_handlers, QHostAddress(ip_address), target_port);
	}
}

void IpcClientNet::WaitConnection()
{
	m_transport->WaitConnection();
}

IpcCall& IpcClientNet::Call()
{
	return *m_ipc_call;
}

bool IpcClientNet::IsExist()
{
	return s_instance;
}

IpcCall& IpcClientNet::Get()
{
	return_x_if_equal(s_instance, NULL, *(reinterpret_cast<IpcCall*>(NULL)));
	return s_instance->Call();
}

void IpcClientNet::OnPostInterThread()
{
}

void IpcClientNet::SendLog(const char* msg, int msg_char_count)
{
	if( AVGetCurrentThreadId() == m_constructor_thread_id )
	{
		std::vector<std::string> thread_messages;
		{
			SynchroLockSection ss(&m_thread_messages_lock);
			thread_messages = m_thread_messages;
			m_thread_messages = std::vector<std::string>();
		}

		for( size_t i = 0; i<thread_messages.size(); i++ )
			Call()("Log")(thread_messages[i]);

		Call()("Log")(std::string(msg, msg_char_count));
	}
	else
	{
		SynchroLockSection ss(&m_thread_messages_lock);
		m_thread_messages.push_back(std::string(msg, msg_char_count));
	}

	
}
