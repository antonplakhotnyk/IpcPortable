#include "IpcClientNet.h"

// IpcClientNet* IpcClientNet::s_instance=nullptr;

IpcClientNet::IpcClientNet()
	:m_constructor_thread_id(ThreadCallerQt::GetCurrentThreadId())
{
// 	mass_assert_if_not_equal(s_instance,nullptr);
// 	s_instance = this;

	m_transport = std::make_shared<IpcClientTcpTransport>();
	m_thread_transport = std::make_shared<IpcThreadTransportQt>();
	m_ipc_call = std::make_unique<MAssIpcCall>(m_transport, m_thread_transport);

	QObject::connect(m_transport.get(), &IpcTcpTransportQt::HandlerProcessTransport, this, std::bind(&MAssIpcCall::ProcessTransport, m_ipc_call.get()));
}

IpcClientNet::~IpcClientNet(void)
{
// 	if( s_instance==this )
// 		s_instance = nullptr;
}

void IpcClientNet::Init(const char* remote_address, uint16_t target_port)
{
	m_transport->Init(remote_address, target_port);
}

void IpcClientNet::WaitConnection()
{
	m_transport->WaitConnection();
}

MAssIpcCall& IpcClientNet::Ipc()
{
	return *m_ipc_call;
}

// bool IpcClientNet::IsExist()
// {
// 	return s_instance;
// }
// 
// MAssIpcCall& IpcClientNet::Get()
// {
// 	mass_return_x_if_equal(s_instance, nullptr, *((MAssIpcCall*)(nullptr)));
// 	return s_instance->Ipc();
// }

IpcTcpTransportQt& IpcClientNet::GetTransportQt()
{
	return *m_transport.get();
}

