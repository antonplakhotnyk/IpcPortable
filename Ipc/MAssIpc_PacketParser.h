#pragma once

#include <vector>
#include <memory>
#include "MAssIpc_Transport.h"
#include "MAssIpc_DataStream.h"

namespace MAssIpcImpl
{

class MAssIpc_PacketParser
{
public:

	typedef uint32_t CallId;

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
	static constexpr int c_net_call_packet_header_size = sizeof(MAssIpc_Data::PacketSize) + sizeof(PacketType) + sizeof(CallId);
	static constexpr MAssIpc_Data::PacketSize c_invalid_packet_size = -1;
	static constexpr CallId c_invalid_id = 0;


	MAssIpc_PacketParser();
	~MAssIpc_PacketParser();

	size_t				ReadNeededDataSize(const std::shared_ptr<MAssIpc_TransportCopy>& in_data, std::unique_ptr<const MAssIpc_Data>* packet_data);

	struct Header
	{
		MAssIpc_Data::PacketSize size = c_invalid_packet_size;
		PacketType type = PacketType::pt_undefined;
		CallId id = c_invalid_id;
	};


	static Header	ReadHeader(MAssIpc_DataStream& connection_stream);
	static void		PacketHeaderWrite(MAssIpc_DataStream& packet_data, MAssIpc_Data::PacketSize no_header_size, MAssIpc_PacketParser::PacketType pt, MAssIpc_PacketParser::CallId id);

private:
	void ReceiveReadHeader(const std::shared_ptr<MAssIpc_TransportCopy>& in_data);

private:

	std::unique_ptr<MAssIpc_Data> m_incoming_packet_header_data;
	struct ReadState
	{
		MAssIpc_Data::PacketSize data_size = c_invalid_packet_size;
		MAssIpc_Data::PacketSize need_size = c_net_call_packet_header_size;

		uint8_t raw_header_buffer[c_net_call_packet_header_size] = {};
	};

	ReadState m_incoming_packet;
};

}