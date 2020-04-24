#include "MAssIpcCallPacket.h"
#include "MAssIpcCallDataStream.h"


MAssIpcCallPacket::MAssIpcCallPacket()
	:m_incoming_packet_data_size(c_invalid_packet_size)
{
}


MAssIpcCallPacket::~MAssIpcCallPacket()
{
}

size_t MAssIpcCallPacket::IsNeedMoreDataSize(const std::shared_ptr<MAssIpcCallTransport>& in_data)
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
			return need_size-available;
		return 0;
	}

	return sizeof(m_incoming_packet_data_size)+c_net_call_packet_type_size;
}

MAssIpcCallPacket::PacketType MAssIpcCallPacket::ReadType(const std::shared_ptr<MAssIpcCallTransport>& in_data)
{
	uint32_t type_raw;

	uint8_t data_raw[sizeof(type_raw)];
	in_data->Read(data_raw, sizeof(data_raw));
	MAssIpcCallDataStream connection_stream(data_raw, sizeof(data_raw));

	connection_stream>>type_raw;
	return (PacketType)type_raw;
}

size_t	MAssIpcCallPacket::FinishReceivePacketSize()
{
	auto res = m_incoming_packet_data_size;
	m_incoming_packet_data_size = MAssIpcCallPacket::c_invalid_packet_size;
	return res;
}

void MAssIpcCallPacket::ReadData(const std::shared_ptr<MAssIpcCallTransport>& in_data, std::vector<uint8_t>* data_replace, size_t data_size)
{
	data_replace->resize(data_size);
	if( data_size!=0 )
		in_data->Read(&(*data_replace)[0], data_size);
}

void MAssIpcCallPacket::SendData(const std::vector<uint8_t>& out_data_buf, PacketType type, std::shared_ptr<MAssIpcCallTransport> out_data)
{
	std::vector<uint8_t> out_data_raw_buf;
	MAssIpcCallDataStream out_data_raw(&out_data_raw_buf);

	out_data_raw<<uint32_t(out_data_buf.size())<<uint32_t(type);
	if( !out_data_buf.empty() )
		out_data_raw.WriteRawData(&out_data_buf[0], out_data_buf.size());
	
	out_data->Write(&out_data_raw_buf[0], out_data_raw_buf.size());
}
