#pragma once

#include <vector>
#include <memory>
#include "MAssIpcCallTransport.h"
#include "MAssIpcCallDataStream.h"

namespace MAssIpcCallInternal
{

class MAssIpcPacketParser
{
public:

	typedef uint32_t TCallId;

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
	static constexpr int c_net_call_packet_header_size = sizeof(MAssIpcData::TPacketSize) + sizeof(PacketType) + sizeof(TCallId);
	static constexpr MAssIpcData::TPacketSize c_invalid_packet_size = -1;
	static constexpr TCallId c_invalid_id = 0;


	MAssIpcPacketParser();
	~MAssIpcPacketParser();

	size_t				ReadNeededDataSize(const std::shared_ptr<MAssIpcCallTransport>& in_data, std::unique_ptr<MAssIpcData>* packet_data);

	struct Header
	{
		MAssIpcData::TPacketSize size = c_invalid_packet_size;
		PacketType type = PacketType::pt_undefined;
		TCallId id = c_invalid_id;
	};


	static Header	ReadHeader(MAssIpcCallDataStream& connection_stream);
	static void		PacketHeaderWrite(MAssIpcCallDataStream& packet_data, MAssIpcData::TPacketSize no_header_size, MAssIpcPacketParser::PacketType pt, MAssIpcPacketParser::TCallId id);

private:
	void ReceiveReadHeader(const std::shared_ptr<MAssIpcCallTransport>& in_data);

private:

	std::unique_ptr<MAssIpcData> m_incoming_packet_header_data;
	MAssIpcData::TPacketSize m_incoming_packet_size = c_invalid_packet_size;
};

}