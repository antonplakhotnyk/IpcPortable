#pragma once

#include <vector>
#include <memory>
#include "MAssIpcCallTransport.h"

namespace MAssIpcCallInternal
{

class MAssIpcCallPacket
{
public:

	// |-- size 4B --|-- type 4B --|-- data (size not including header) --|
	enum PacketType:uint32_t
	{
		pt_call, pt_return, pt_return_fail_call, pt_enumerate, pt_enumerate_return
	};
	static const int c_net_call_packet_type_size = 4;
	static const int c_net_call_packet_header_size = 8;
	static const uint32_t c_invalid_packet_size = -1;


	MAssIpcCallPacket();
	~MAssIpcCallPacket();

	size_t				IsNeedMoreDataSize(const std::shared_ptr<MAssIpcCallTransport>& in_data);
	PacketType			ReadType(const std::shared_ptr<MAssIpcCallTransport>& in_data);
	void				ReadData(const std::shared_ptr<MAssIpcCallTransport>& in_data, std::vector<uint8_t>* data_replace, size_t data_size);
	size_t				FinishReceivePacketSize();

	//	static void SendData(const std::vector<uint8_t>& out_data_buf, PacketType type, std::shared_ptr<MAssIpcCallTransport> out_data);

	static void PacketHeaderAllocate(std::vector<uint8_t>* packet_data, MAssIpcCallPacket::PacketType pt);
	static void PacketHeaderUpdateSize(std::vector<uint8_t>* packet_data);

private:

	uint32_t	m_incoming_packet_data_size;
};

}