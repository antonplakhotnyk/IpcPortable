#pragma once

#include <stdint.h>
#include <vector>
#include <memory>
#include "MAssIpcCallTransport.h"

class MAssIpcCallPacket
{
public:

	// |-- size 4B --|-- type 4B --|-- data (size B) --|
	enum PacketType:uint32_t { pt_call, pt_return, pt_enumerate, pt_enumerate_return };
	static const int c_net_call_packet_type_size = 4;
	static const int c_net_call_packet_header_size = 8;


	MAssIpcCallPacket();
	~MAssIpcCallPacket();

	size_t				IsNeedMoreDataSize(const std::shared_ptr<MAssIpcCallTransport>& in_data);
	PacketType			ReadType(const std::shared_ptr<MAssIpcCallTransport>& in_data);
	void				ReadData(const std::shared_ptr<MAssIpcCallTransport>& in_data, std::vector<uint8_t>* data_replace);

	static void SendData(const std::vector<uint8_t>& out_data_buf, PacketType type, std::shared_ptr<MAssIpcCallTransport> out_data);

private:

	uint32_t	m_packet_size;
};

