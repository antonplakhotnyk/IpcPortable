#include "MAssIpcRawString.h"

namespace MAssIpcCallInternal
{

MAssIpcRawString MAssIpcRawString::Read(MAssIpcCallDataStream& stream)
{
	MAssIpcData::TPacketSize length = 0;
	stream>>length;
	const char* raw_data = nullptr;
	if( length!=0 )
		raw_data = stream.ReadRawDataChar(length/sizeof(char));
	return {raw_data,length};
}

void MAssIpcRawString::Write(MAssIpcCallDataStream& stream) const
{
	const MAssIpcData::TPacketSize length = (m_len*sizeof(char));
	stream<<length;
	stream.WriteRawData(reinterpret_cast<const uint8_t*>(m_str), length);
}

}