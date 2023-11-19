#include "MAssIpc_RawString.h"
#include "MAssIpc_Macros.h"

namespace MAssIpcImpl
{

MAssIpc_RawString MAssIpc_RawString::Read(MAssIpc_DataStream& stream)
{
	MAssIpc_Data::PacketSize length = 0;
	stream>>length;
	const char* raw_data = nullptr;
	if( length!=0 )
		raw_data = stream.ReadRawDataChar(length/sizeof(char));
	return {raw_data,length};
}

void MAssIpc_RawString::Write(MAssIpc_DataStream& stream) const
{
	const MAssIpc_Data::PacketSize length = (m_len*sizeof(char));
	stream<<length;
	stream.WriteRawData(reinterpret_cast<const uint8_t*>(m_str), length);
}

MAssIpc_Data::PacketSize MAssIpc_RawString::ConvertCheckStrLen(size_t str_len)
{
	MAssIpc_Data::PacketSize res = MAssIpc_Data::PacketSize(str_len);
	mass_return_x_if_not_equal(res, str_len, 0);
	return res;
}

}