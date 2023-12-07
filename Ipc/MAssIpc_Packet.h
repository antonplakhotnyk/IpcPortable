#pragma once

namespace MAssIpcImpl
{

struct MAssIpc_Packet
{
	using PacketSize=uint32_t;
	using CallId=uint32_t;

	// |-- size 4B --|-- type 4B --|-- id 4B --|-- data (size not including header) --|
	enum struct PacketType:uint32_t
	{
		pt_undefined,
		pt_enumerate,
		pt_call,// this and before id's processed as incoming calls
		pt_return,// this and subsequent id's processed as responses
		pt_return_fail_call,
		pt_return_enumerate,
		pt_count_not_a_type,// for calculating count of values
	};
	static constexpr PacketSize c_net_call_packet_header_size=sizeof(PacketSize) + sizeof(PacketType) + sizeof(CallId);
	static constexpr PacketSize c_invalid_packet_size=-1;
	static constexpr CallId c_invalid_id=0;

};

}