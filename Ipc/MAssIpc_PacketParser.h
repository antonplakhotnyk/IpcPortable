#pragma once

#include <vector>
#include <memory>
#include "MAssIpc_Transport.h"
#include "MAssIpc_DataStream.h"
#include "MAssIpc_Packet.h"

namespace MAssIpcImpl
{

class MAssIpc_PacketParser: public MAssIpc_Packet
{
public:

	MAssIpc_PacketParser();
	~MAssIpc_PacketParser();

	size_t				ReadNeededDataSize(const std::shared_ptr<MAssIpc_TransportCopy>& in_data, std::unique_ptr<const MAssIpc_Data>* packet_data);

	struct Header
	{
		PacketSize size = c_invalid_packet_size;
		PacketType type = PacketType::pt_undefined;
		CallId id = c_invalid_id;
	};


	static Header	ReadHeader(MAssIpc_DataStream& connection_stream);
	static void		PacketHeaderWrite(MAssIpc_DataStream& packet_data, PacketSize no_header_size, MAssIpc_PacketParser::PacketType pt, MAssIpc_PacketParser::CallId id);

private:
	void ReceiveReadHeader(const std::shared_ptr<MAssIpc_TransportCopy>& in_data);
	
	enum struct ReadDataStatus
	{
		available,
		nodata,
		failed,
	};
	ReadDataStatus ReadDataIsAvailable(const std::shared_ptr<MAssIpc_TransportCopy>& in_data, uint8_t* buffer, PacketSize incoming_packet_data_size);


private:

	std::unique_ptr<MAssIpc_Data> m_incoming_packet_header_data;
	struct ReadState
	{
		PacketSize data_size = c_invalid_packet_size;
		PacketSize need_size = c_net_call_packet_header_size;

		uint8_t raw_header_buffer[c_net_call_packet_header_size] = {};
	};

	ReadState m_incoming_packet;
};

}