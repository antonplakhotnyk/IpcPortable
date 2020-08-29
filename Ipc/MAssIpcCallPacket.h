#pragma once

#include <vector>
#include <memory>
#include "MAssIpcCallTransport.h"

namespace MAssIpcCallInternal
{

class MAssIpcCallPacket
{
public:

	typedef uint32_t TCallId;
	typedef uint32_t TPacketSize;

	// |-- size 4B --|-- type 4B --|-- id 4B --|-- data (size not including header) --|
	enum struct PacketType:uint32_t
	{
		pt_undefined, 
		pt_enumerate,
		pt_call,// this and before id's processed as incoming calls
		pt_return, // this and subsequent id's processed as responces
		pt_return_fail_call, 
		pt_return_enumerate,
		pt_count_not_a_type,// for calculating count of values
	};
	static constexpr int c_net_call_packet_type_size = sizeof(PacketType);
	static constexpr int c_net_call_packet_id_size = sizeof(TCallId);
	static constexpr int c_net_call_packet_header_size = sizeof(TPacketSize) + c_net_call_packet_type_size + c_net_call_packet_id_size;
	static constexpr int c_offset_header_size = 0;
	static constexpr int c_offset_header_id = sizeof(TPacketSize) + sizeof(PacketType);
	static constexpr TPacketSize c_invalid_packet_size = -1;
	static constexpr TCallId c_invalid_id = 0;


	MAssIpcCallPacket();
	~MAssIpcCallPacket();

	size_t				ReadNeededDataSize(const std::shared_ptr<MAssIpcCallTransport>& in_data);
	void				ReadData(const std::shared_ptr<MAssIpcCallTransport>& in_data, std::vector<uint8_t>* data_replace, size_t data_size);

	struct Header
	{
		TPacketSize size = 0;
		PacketType type = PacketType::pt_undefined;
		TCallId id = c_invalid_id;
	};

	Header FinishReceiveReadHeader(const std::shared_ptr<MAssIpcCallTransport>& in_data);

	static void PacketHeaderAllocate(std::vector<uint8_t>* packet_data, MAssIpcCallPacket::PacketType pt, MAssIpcCallPacket::TCallId id);
	static void PacketHeaderUpdateSize(std::vector<uint8_t>* packet_data);

private:

	uint32_t	m_incoming_packet_data_size;
};

}