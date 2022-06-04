#include "MAssIpc_PacketParser.h"
#include "MAssIpc_Macros.h"
#include "MAssIpc_Internals.h "
#include <cstring>

namespace MAssIpcCallInternal
{


MAssIpc_PacketParser::MAssIpc_PacketParser()
{
}


MAssIpc_PacketParser::~MAssIpc_PacketParser()
{
}

size_t MAssIpc_PacketParser::ReadNeededDataSize(const std::shared_ptr<MAssIpc_TransportCopy>& in_data, std::unique_ptr<const MAssIpc_Data>* packet_data)
{
	if( m_incoming_packet_size == c_invalid_packet_size )
		if( in_data->ReadBytesAvailable()>=c_net_call_packet_header_size )
			ReceiveReadHeader(in_data);

	if( (m_incoming_packet_size != c_invalid_packet_size) )
	{
		const size_t available = in_data->ReadBytesAvailable();
		const size_t need_size = m_incoming_packet_size;
		if( available < need_size )
			return need_size;
		{
			if( m_incoming_packet_size!=0 )
				in_data->Read(m_incoming_packet_header_data->Data()+c_net_call_packet_header_size, m_incoming_packet_size);

			*packet_data = std::move(m_incoming_packet_header_data);
			m_incoming_packet_size = c_invalid_packet_size;
		}
		return 0;
	}

	return c_net_call_packet_header_size;
}

void MAssIpc_PacketParser::ReceiveReadHeader(const std::shared_ptr<MAssIpc_TransportCopy>& in_data)
{
	uint8_t data_raw[c_net_call_packet_header_size] = {};
	in_data->Read(data_raw, sizeof(data_raw));

	m_incoming_packet_size = MAssIpc_DataStream::ReadUnsafe<decltype(m_incoming_packet_size)>(data_raw);

	// sanity check? type? size? crc?
	m_incoming_packet_header_data = std::make_unique<MAssIpcData_Vector>(c_net_call_packet_header_size+m_incoming_packet_size);
	memcpy(m_incoming_packet_header_data->Data(), data_raw, sizeof(data_raw));
}

MAssIpc_PacketParser::Header MAssIpc_PacketParser::ReadHeader(MAssIpc_DataStream& connection_stream)
{
	Header header;
	connection_stream>>header.size;

	{
		std::underlying_type<PacketType>::type type_raw;
		connection_stream>>type_raw;
		header.type = (PacketType)type_raw;
	}

	connection_stream>>header.id;
	return header;
}

void MAssIpc_PacketParser::PacketHeaderWrite(MAssIpc_DataStream& packet_data,
											   MAssIpc_Data::TPacketSize no_header_size,
											   MAssIpc_PacketParser::PacketType pt,
											   MAssIpc_PacketParser::TCallId id)
{
	packet_data<<MAssIpc_Data::TPacketSize(no_header_size)<<std::underlying_type<PacketType>::type(pt)<<TCallId(id);
}

}