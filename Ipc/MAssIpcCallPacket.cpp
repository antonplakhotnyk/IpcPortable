#include "MAssIpcCallPacket.h"
#include "MAssIpcCallDataStream.h"
#include "MAssMacros.h"
#include <limits>

using namespace MAssIpcCallInternal;


MAssIpcCallPacket::MAssIpcCallPacket()
	:m_incoming_packet_data_size(c_invalid_packet_size)
{
}


MAssIpcCallPacket::~MAssIpcCallPacket()
{
}

size_t MAssIpcCallPacket::ReadNeededDataSize(const std::shared_ptr<MAssIpcCallTransport>& in_data)
{
	if( m_incoming_packet_data_size==c_invalid_packet_size )
	{
		if( in_data->ReadBytesAvailable()>=sizeof(m_incoming_packet_data_size)+c_net_call_packet_type_size )
		{
			uint8_t data_raw[sizeof(m_incoming_packet_data_size)];
			in_data->Read(data_raw, sizeof(data_raw));
			MAssIpcCallDataStream connection_stream(data_raw, sizeof(data_raw));

			connection_stream>>m_incoming_packet_data_size;
		}
	}
		
	if( m_incoming_packet_data_size!=c_invalid_packet_size )
	{
		size_t available = in_data->ReadBytesAvailable();
		size_t need_size = (m_incoming_packet_data_size+c_net_call_packet_type_size);
		if( available < need_size )
			return need_size;
		return 0;
	}

	return sizeof(m_incoming_packet_data_size)+c_net_call_packet_type_size;
}

MAssIpcCallPacket::Header MAssIpcCallPacket::FinishReceiveReadHeader(const std::shared_ptr<MAssIpcCallTransport>& in_data)
{
	uint8_t data_raw[sizeof(PacketType)+sizeof(TCallId)];
	in_data->Read(data_raw, sizeof(data_raw));
	MAssIpcCallDataStream connection_stream(data_raw, sizeof(data_raw));

	Header res;
	res.size = m_incoming_packet_data_size;

	{
		std::underlying_type<PacketType>::type type_raw;
		connection_stream>>type_raw;
		res.type = (PacketType)type_raw;
	}

	connection_stream>>res.id;

	m_incoming_packet_data_size = MAssIpcCallPacket::c_invalid_packet_size;
	return res;
}

void MAssIpcCallPacket::ReadData(const std::shared_ptr<MAssIpcCallTransport>& in_data, std::vector<uint8_t>* data_replace, size_t data_size)
{
	data_replace->resize(data_size);
	if( data_size!=0 )
		in_data->Read(&(*data_replace)[0], data_size);
}

void MAssIpcCallPacket::PacketHeaderAllocate(std::vector<uint8_t>* packet_data, MAssIpcCallPacket::PacketType pt, MAssIpcCallPacket::TCallId id)
{
	MAssIpcCallDataStream out_data_raw(packet_data);
	out_data_raw<<TPacketSize(0)<<std::underlying_type<PacketType>::type(pt)<<TCallId(id);
}

void MAssIpcCallPacket::PacketHeaderUpdateSize(std::vector<uint8_t>* packet_data)
{
	mass_return_if_equal(packet_data->size()<MAssIpcCallPacket::c_net_call_packet_header_size, true);
	uint8_t* packet_data_bytes = packet_data->data();
	size_t packet_data_size = packet_data->size() - MAssIpcCallPacket::c_net_call_packet_header_size;
	mass_return_if_equal(std::numeric_limits<TPacketSize>::max() < packet_data_size, true);
	TPacketSize header_packet_data_size = packet_data_size;
	MAssIpcCallDataStream::WriteUnsafe(packet_data_bytes+c_offset_header_size, header_packet_data_size);
}

