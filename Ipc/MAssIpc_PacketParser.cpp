#include "MAssIpc_PacketParser.h"
#include "MAssIpc_Internals.h"
#include "MAssIpc_Macros.h"
#include <cstring>

namespace MAssIpcImpl
{


MAssIpc_PacketParser::MAssIpc_PacketParser()
{
}


MAssIpc_PacketParser::~MAssIpc_PacketParser()
{
}

MAssIpc_PacketParser::ReadDataStatus MAssIpc_PacketParser::ReadDataIsAvailable(const std::shared_ptr<MAssIpc_TransportCopy>& in_data, uint8_t* buffer, PacketSize incoming_packet_data_size)
{
	const size_t available=in_data->ReadBytesAvailable();
	if( available > 0 )
	{
		const auto read_size{PacketSize(std::min(size_t(m_incoming_packet.need_size), available))};
		if( read_size!=0 )
		{
			const auto read_offset{incoming_packet_data_size - m_incoming_packet.need_size};
			if( !in_data->Read(buffer+ read_offset, read_size) )
				return ReadDataStatus::failed;
			m_incoming_packet.need_size-=read_size;
		}

		return ReadDataStatus::available;
	}

	return ReadDataStatus::nodata;
}

size_t MAssIpc_PacketParser::ReadNeededDataSize(const std::shared_ptr<MAssIpc_TransportCopy>& in_data, std::unique_ptr<const MAssIpc_Data>* packet_data)
{
	if( m_incoming_packet.data_size == c_invalid_packet_size )
	{
		const ReadDataStatus is_available = ReadDataIsAvailable(in_data, m_incoming_packet.raw_header_buffer, c_net_call_packet_header_size);
		return_x_if_equal_mass_ipc(is_available, ReadDataStatus::failed, c_invalid_packet_size);
		if( is_available==ReadDataStatus::available && (m_incoming_packet.need_size==0) )
			ReceiveReadHeader(in_data);
	}

	if( (m_incoming_packet.data_size != c_invalid_packet_size) )
	{
		const ReadDataStatus read_status = ReadDataIsAvailable(in_data, m_incoming_packet_header_data->Data()+c_net_call_packet_header_size, m_incoming_packet.data_size);
		return_x_if_equal_mass_ipc(read_status, ReadDataStatus::failed, c_invalid_packet_size);

		if( m_incoming_packet.need_size==0 )
		{
// 			{// was used for debug uninitialized values
// 				uint8_t* data = m_incoming_packet_header_data->Data();
// 				size_t size = m_incoming_packet_header_data->Size();
// 				if( size>=4 )
// 					if( (data[size-1] == 0xCD)&&(data[size-2] == 0xCD)&&(data[size-3] == 0xCD)&&(data[size-4] == 0xCD) )
// 						_CrtDbgBreak();
// 			}

			*packet_data = std::move(m_incoming_packet_header_data);
			m_incoming_packet = ReadState{};
		}
	}

	return m_incoming_packet.need_size;
}

void MAssIpc_PacketParser::ReceiveReadHeader(const std::shared_ptr<MAssIpc_TransportCopy>& in_data)
{
	m_incoming_packet.data_size = MAssIpc_DataStream::ReadUnsafe<decltype(m_incoming_packet.data_size)>(m_incoming_packet.raw_header_buffer);
	m_incoming_packet.need_size = m_incoming_packet.data_size;

	// sanity check? type? size? crc?
	m_incoming_packet_header_data = std::make_unique<MAssIpcData_Vector>(c_net_call_packet_header_size+m_incoming_packet.data_size);
	memcpy(m_incoming_packet_header_data->Data(), m_incoming_packet.raw_header_buffer, sizeof(m_incoming_packet.raw_header_buffer));
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
											   PacketSize no_header_size,
											   MAssIpc_PacketParser::PacketType pt,
											   MAssIpc_PacketParser::CallId id)
{
	packet_data<<PacketSize(no_header_size)<<std::underlying_type<PacketType>::type(pt)<<CallId(id);
}

}
