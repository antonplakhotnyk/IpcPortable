#include "IpcWin_TransportTcpClient.h"
#include <stdexcept>
#include <iostream>

#include "MAssIpc_Internals.h"

//-------------------------------------------------------


IpcWin_TransportTcpClient_Copy::IpcWin_TransportTcpClient_Copy()
{
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	return_if_equal(m_socket, INVALID_SOCKET);
}

IpcWin_TransportTcpClient_Copy::~IpcWin_TransportTcpClient_Copy()
{
	if( m_socket != INVALID_SOCKET)
		closesocket(m_socket);
}

bool IpcWin_TransportTcpClient_Copy::Connect(const std::string& address, uint16_t port)
{
	addrinfo hints = {};
	hints.ai_family = AF_INET;       // IPv4
	hints.ai_socktype = SOCK_STREAM; // Stream socket
	hints.ai_protocol = IPPROTO_TCP; // TCP

	addrinfo* result = nullptr;
	int addrinfo_res = getaddrinfo(address.c_str(), nullptr, &hints, &result);
	return_x_if_not_equal(addrinfo_res, 0, false);

	sockaddr_in server_addr = {};
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr = reinterpret_cast<sockaddr_in*>(result->ai_addr)->sin_addr;

	freeaddrinfo(result); // Free the address info structure

	int connect_res = connect(m_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	return_x_if_equal(connect_res, SOCKET_ERROR, false);

	return true;
}

bool IpcWin_TransportTcpClient_Copy::WaitRespond(size_t expected_size)
{
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(m_socket, &readfds);
	timeval timeout = {30, 0}; // 5 seconds timeout
	int result = select(0, &readfds, nullptr, nullptr, &timeout);
	return result > 0;
}

size_t IpcWin_TransportTcpClient_Copy::ReadBytesAvailable()
{
	u_long available = 0;
	ioctlsocket(m_socket, FIONREAD, &available);
	return available;
}

bool IpcWin_TransportTcpClient_Copy::Read(uint8_t* data, size_t size)
{
	int received = recv(m_socket, reinterpret_cast<char*>(data), size, 0);
	return_x_if_equal_msg(received, SOCKET_ERROR, false, "Socket read error");
	return true;
}

void IpcWin_TransportTcpClient_Copy::Write(const uint8_t* data, size_t size)
{
	int sent = send(m_socket, reinterpret_cast<const char*>(data), size, 0);
	return_if_equal_msg(sent, SOCKET_ERROR, "Socket write error");
}

IpcWin_TransportTcpClient_Share::IpcWin_TransportTcpClient_Share(const std::shared_ptr<IpcWin_TransportTcpClient_Copy>& transport_copy)
	: m_transport_copy(transport_copy)
{
}

std::unique_ptr<MAssIpc_Data> IpcWin_TransportTcpClient_Share::Create(MAssIpc_Data::PacketSize size)
{
	return std::make_unique<MAssIpcImpl::MAssIpcData_Vector>(size);
}

void IpcWin_TransportTcpClient_Share::Write(std::unique_ptr<const MAssIpc_Data> packet)
{
	m_transport_copy->Write(packet->Data(), packet->Size());
}

bool IpcWin_TransportTcpClient_Share::Read(bool wait_incoming_packet, std::unique_ptr<const MAssIpc_Data>* packet)
{
	if( wait_incoming_packet )
	{
		m_transport_copy->WaitRespond(0);
	}
	size_t available = m_transport_copy->ReadBytesAvailable();
	if( available == 0 )
	{
		return false;
	}
	auto data = std::make_unique<MAssIpcImpl::MAssIpcData_Vector>(available);
	m_transport_copy->Read(data->Data(), available);
	*packet = std::move(data);
	return true;
}

bool IpcWin_TransportTcpClient_Share::WaitRespond(size_t expected_size)
{
	return m_transport_copy->WaitRespond(expected_size);
}
