#include "MAssIpc_RawString.h"
#include "MAssIpc_Macros.h"

namespace MAssIpcImpl
{

RawString RawString::Read(MAssIpc_DataStream& stream)
{
	MAssIpc_Data::PacketSize length = 0;
	stream>>length;
	const char* raw_data = nullptr;
	if( length!=0 )
		raw_data = stream.ReadRawDataChar(length/sizeof(char));
	return {raw_data,length};
}

void RawString::Write(MAssIpc_DataStream& stream) const
{
	const MAssIpc_Data::PacketSize length = (m_len*sizeof(char));
	stream<<length;
	stream.WriteRawData(reinterpret_cast<const uint8_t*>(m_str), length);
}

MAssIpc_Data::PacketSize RawString::ConvertCheckStrLen(size_t str_len)
{
	MAssIpc_Data::PacketSize res = MAssIpc_Data::PacketSize(str_len);
	return_x_if_not_equal_mass_ipc(res, str_len, 0);
	return res;
}

}