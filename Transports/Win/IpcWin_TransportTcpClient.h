#pragma once

#include "MAssIpc_Transport.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include "IpcWin_WsaStartup.h"
#include <memory>
#include <vector>



//-------------------------------------------------------

class IpcWin_TransportTcpClient_Copy: public MAssIpc_TransportCopy
{
public:
	IpcWin_TransportTcpClient_Copy();
	~IpcWin_TransportTcpClient_Copy();

	bool Connect(const std::string& address, uint16_t port);

	bool WaitRespond(size_t expected_size) override;
	size_t ReadBytesAvailable() override;
	bool Read(uint8_t* data, size_t size) override;
	void Write(const uint8_t* data, size_t size) override;

private:
	SOCKET m_socket = INVALID_SOCKET;
	std::vector<uint8_t> m_read_buffer;
	size_t m_read_offset = 0;
	
	std::shared_ptr<IpcWin_WsaStartup> m_wsa_startup = IpcWin_WsaStartup::Retain();
};

//-------------------------------------------------------

class IpcWin_TransportTcpClient_Share: public MAssIpc_TransportShare
{
public:
	IpcWin_TransportTcpClient_Share(const std::shared_ptr<IpcWin_TransportTcpClient_Copy>& transport_copy);

	std::unique_ptr<MAssIpc_Data> Create(MAssIpc_Data::PacketSize size) override;
	void Write(std::unique_ptr<const MAssIpc_Data> packet) override;
	bool Read(bool wait_incoming_packet, std::unique_ptr<const MAssIpc_Data>* packet) override;

	bool WaitRespond(size_t expected_size);

private:
	std::shared_ptr<IpcWin_TransportTcpClient_Copy> m_transport_copy;
};

//-------------------------------------------------------
