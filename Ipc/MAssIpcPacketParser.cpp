#include "MAssIpcPacketParser.h"
#include "MAssMacros.h"
#include "MAssIpcCallInternal.h"
#include <cstring>

namespace MAssIpcCallInternal
{


MAssIpcPacketParser::MAssIpcPacketParser()
{
}


MAssIpcPacketParser::~MAssIpcPacketParser()
{
}

size_t MAssIpcPacketParser::ReadNeededDataSize(const std::shared_ptr<MAssIpcCallTransport>& in_data, std::unique_ptr<MAssIpcData>* packet_data)
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

void MAssIpcPacketParser::ReceiveReadHeader(const std::shared_ptr<MAssIpcCallTransport>& in_data)
{
	uint8_t data_raw[c_net_call_packet_header_size] = {};
	in_data->Read(data_raw, sizeof(data_raw));

	m_incoming_packet_size = MAssIpcCallDataStream::ReadUnsafe<decltype(m_incoming_packet_size)>(data_raw);

	// sanity check? type? size? crc?
	m_incoming_packet_header_data.reset(new MAssIpcData_Vector(c_net_call_packet_header_size+m_incoming_packet_size));
	memcpy(m_incoming_packet_header_data->Data(), data_raw, sizeof(data_raw));
}

MAssIpcPacketParser::Header MAssIpcPacketParser::ReadHeader(MAssIpcCallDataStream& connection_stream)
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

void MAssIpcPacketParser::PacketHeaderWrite(MAssIpcCallDataStream& packet_data,
											   MAssIpcData::TPacketSize no_header_size,
											   MAssIpcPacketParser::PacketType pt,
											   MAssIpcPacketParser::TCallId id)
{
	packet_data<<MAssIpcData::TPacketSize(no_header_size)<<std::underlying_type<PacketType>::type(pt)<<TCallId(id);
}

}