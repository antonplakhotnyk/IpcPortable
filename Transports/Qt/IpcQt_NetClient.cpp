#include "IpcQt_NetClient.h"

// IpcQt_NetClient* IpcQt_NetClient::s_instance=nullptr;

IpcQt_NetClient::IpcQt_NetClient(MAssIpcCall& ipc_connection)
	:m_transport_client(std::make_shared<IpcQt_TransportTcpClient>())
	, m_ipc_net(ipc_connection, m_transport_client)
{

// 	mass_assert_if_not_equal(s_instance,nullptr);
// 	s_instance = this;
}

IpcQt_NetClient::~IpcQt_NetClient(void)
{
// 	if( s_instance==this )
// 		s_instance = nullptr;
}

void IpcQt_NetClient::WaitConnection()
{
	m_transport_client->WaitConnection();
}

// bool IpcQt_NetClient::IsExist()
// {
// 	return s_instance;
// }
// 
// MAssIpcCall& IpcQt_NetClient::Get()
// {
// 	mass_return_x_if_equal(s_instance, nullptr, *((MAssIpcCall*)(nullptr)));
// 	return s_instance->Ipc();
// }

IpcQt_TransportTcpClient& IpcQt_NetClient::GetIpcClientTcpTransport()
{
	return *m_transport_client.get();
}

