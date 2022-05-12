#include "IpcClientNet.h"

// IpcClientNet* IpcClientNet::s_instance=nullptr;

IpcClientNet::IpcClientNet()
	:m_transport(std::make_shared<IpcClientTcpTransport>())
	, m_ipc_net(m_transport)
{
// 	mass_assert_if_not_equal(s_instance,nullptr);
// 	s_instance = this;
}

IpcClientNet::~IpcClientNet(void)
{
// 	if( s_instance==this )
// 		s_instance = nullptr;
}

void IpcClientNet::Init(const IpcClientTcpTransport::Addr& addr)
{
	m_transport->Init(addr);
}

void IpcClientNet::WaitConnection()
{
	m_transport->WaitConnection();
}

MAssIpcCall& IpcClientNet::Ipc()
{
	return m_ipc_net.Call();
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

