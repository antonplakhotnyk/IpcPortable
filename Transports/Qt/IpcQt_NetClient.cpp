#include "IpcClientNet.h"

// IpcClientNet* IpcClientNet::s_instance=nullptr;

IpcClientNet::IpcClientNet(MAssIpcCall& ipc_connection)
	:m_transport_client(std::make_shared<IpcClientTcpTransport>())
	, m_ipc_net(ipc_connection, m_transport_client)
{

// 	mass_assert_if_not_equal(s_instance,nullptr);
// 	s_instance = this;
}

IpcClientNet::~IpcClientNet(void)
{
// 	if( s_instance==this )
// 		s_instance = nullptr;
}

void IpcClientNet::WaitConnection()
{
	m_transport_client->WaitConnection();
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

IpcClientTcpTransport& IpcClientNet::GetIpcClientTcpTransport()
{
	return *m_transport_client.get();
}

