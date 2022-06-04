#include "MAssIpcCallDataStream.h"
#include <cstring>
#include "MAssMacros.h"
#include <limits>


MAssIpcCallDataStream::MAssIpcCallDataStream(std::unique_ptr<const MAssIpcData> data_read)
	:m_read(std::move(data_read))
{
}

MAssIpcCallDataStream::MAssIpcCallDataStream(std::unique_ptr<MAssIpcData> data_write)
	:m_write(std::move(data_write))
{
}

MAssIpcCallDataStream::~MAssIpcCallDataStream()
{
}

bool MAssIpcCallDataStream::IsReadBufferPresent()
{
	return bool(m_read);
}

std::unique_ptr<const MAssIpcData> MAssIpcCallDataStream::DetachRead()
{
	return std::move(m_read);
}

std::unique_ptr<MAssIpcData> MAssIpcCallDataStream::DetachWrite()
{
	return std::move(m_write);
}

const MAssIpcData* MAssIpcCallDataStream::GetDataRead() const
{
	return m_read.get();
}

MAssIpcData* MAssIpcCallDataStream::GetDataWrite() const
{
	return m_write.get();
}

bool MAssIpcCallDataStream::CheckAssert(bool assert)
{
	mass_return_x_if_equal(assert, true, assert);
	return assert;
}

void MAssIpcCallDataStream::WriteRawData(const uint8_t* data, MAssIpcData::TPacketSize len)
{
	uint8_t* bytes = WriteRawData(len);
	if( bytes )
		memcpy(bytes, data, len);
}

uint8_t* MAssIpcCallDataStream::WriteRawData(MAssIpcData::TPacketSize len)
{
	uint8_t* bytes = nullptr;
	if( m_write )
	{
		MAssIpcData::TPacketSize size = m_write->Size();
		mass_return_x_if_equal((m_write_pos<(std::numeric_limits<MAssIpcData::TPacketSize>::max()-len))&&(size < m_write_pos+len), true, nullptr);

		bytes = m_write->Data()+m_write_pos;
	}

	m_write_pos += len;// calc necessary size even without storage

	return bytes;
}

const uint8_t* MAssIpcCallDataStream::ReadRawData(MAssIpcData::TPacketSize len)
{
	mass_return_x_if_equal(IsReadAvailable(len), false, nullptr);
	const uint8_t* pos = m_read->Data()+m_read_pos;
	m_read_pos += len;
	return pos;
}

void MAssIpcCallDataStream::ReadRawData(uint8_t* data, MAssIpcData::TPacketSize len)
{
	const uint8_t* pos = ReadRawData(len);
	if( pos )
		memcpy(data, pos, len);
	else
		memset(data, 0, len);
}

bool MAssIpcCallDataStream::IsReadAvailable(MAssIpcData::TPacketSize size)
{
	if( m_read)
	{
		bool res = m_read->Size()-m_read_pos >= size;
		return res;
	}

	return false;
}

MAssIpcData::TPacketSize MAssIpcCallDataStream::GetWritePos()
{
	return m_write_pos;
}

//-------------------------------------------------------

void MAssIpcCallDataStream::ReadRawData(char* data, MAssIpcData::TPacketSize len)
{
	uint8_t* u8_data = reinterpret_cast<uint8_t*>(data);
	static_assert(sizeof(*u8_data)==sizeof(*data), "must be same size");
	ReadRawData(u8_data, len);
}

const char* MAssIpcCallDataStream::ReadRawDataChar(MAssIpcData::TPacketSize len)
{
	const uint8_t* u8_data = ReadRawData(len);
	const char* data = reinterpret_cast<const char*>(u8_data);
	static_assert(sizeof(*u8_data)==sizeof(*data), "must be same size");
	return data;
}

void MAssIpcCallDataStream::WriteRawData(const char* data, MAssIpcData::TPacketSize len)
{
	const uint8_t* u8_data = reinterpret_cast<const uint8_t*>(data);
	static_assert(sizeof(*u8_data)==sizeof(*data), "must be same size");
	WriteRawData(u8_data, len);
}

MAssIpcCallDataStream &MAssIpcCallDataStream::operator>>(int8_t &i)
{
	ReadBytes(&i);
	return *this;
}

MAssIpcCallDataStream &MAssIpcCallDataStream::operator>>(int16_t &i)
{
	ReadBytes(&i);
	return *this;
}

MAssIpcCallDataStream &MAssIpcCallDataStream::operator>>(int32_t &i)
{
	ReadBytes(&i);
	return *this;
}

MAssIpcCallDataStream &MAssIpcCallDataStream::operator>>(int64_t &i)
{
	ReadBytes(&i);
	return *this;
}


MAssIpcCallDataStream &MAssIpcCallDataStream::operator>>(bool &i)
{
	ReadRawData(reinterpret_cast<uint8_t*>(&i), sizeof(i));
	return *this;
}

MAssIpcCallDataStream &MAssIpcCallDataStream::operator>>(float &f)
{
	ReadRawData(reinterpret_cast<uint8_t*>(&f), sizeof(f));
	return *this;
}

MAssIpcCallDataStream &MAssIpcCallDataStream::operator>>(double &f)
{
	ReadRawData(reinterpret_cast<uint8_t*>(&f), sizeof(f));
	return *this;
}

MAssIpcCallDataStream &MAssIpcCallDataStream::operator<<(int8_t i)
{
	WriteBytes(i);
	return *this;
}

MAssIpcCallDataStream &MAssIpcCallDataStream::operator<<(int16_t i)
{
	WriteBytes(i);
	return *this;
}

MAssIpcCallDataStream &MAssIpcCallDataStream::operator<<(int32_t i)
{
	WriteBytes(i);
	return *this;
}

MAssIpcCallDataStream &MAssIpcCallDataStream::operator<<(int64_t i)
{
	WriteBytes(i);
	return *this;
}

MAssIpcCallDataStream &MAssIpcCallDataStream::operator<<(bool i)
{
	WriteRawData(reinterpret_cast<uint8_t*>(&i), sizeof(i));
	return *this;
}

MAssIpcCallDataStream &MAssIpcCallDataStream::operator<<(float f)
{
	WriteRawData(reinterpret_cast<uint8_t*>(&f), sizeof(f));
 	return *this;
}

MAssIpcCallDataStream &MAssIpcCallDataStream::operator<<(double f)
{
	WriteRawData(reinterpret_cast<uint8_t*>(&f), sizeof(f));
	return *this;
}


MAssIpcCallDataStream &MAssIpcCallDataStream::operator>>(uint8_t &i)
{ 
	return *this >> reinterpret_cast<int8_t&>(i); 
}

MAssIpcCallDataStream &MAssIpcCallDataStream::operator>>(uint16_t &i)
{ 
	return *this >> reinterpret_cast<int16_t&>(i); 
}

MAssIpcCallDataStream &MAssIpcCallDataStream::operator>>(uint32_t &i)
{ 
	return *this >> reinterpret_cast<int32_t&>(i); 
}

MAssIpcCallDataStream &MAssIpcCallDataStream::operator>>(uint64_t &i)
{ 
	return *this >> reinterpret_cast<int64_t&>(i); 
}

MAssIpcCallDataStream &MAssIpcCallDataStream::operator<<(uint8_t i)
{ 
	return *this << int8_t(i); 
}

MAssIpcCallDataStream &MAssIpcCallDataStream::operator<<(uint16_t i)
{ 
	return *this << int16_t(i); 
}

MAssIpcCallDataStream &MAssIpcCallDataStream::operator<<(uint32_t i)
{ 
	return *this << int32_t(i); 
}

MAssIpcCallDataStream &MAssIpcCallDataStream::operator<<(uint64_t i)
{ 
	return *this << int64_t(i); 
}

//-------------------------------------------------------
// 
// MAssIpcCallDataStream& operator<<(MAssIpcCallDataStream& stream, const int& v)
// {
// 	STATIC_ASSERT(sizeof(v)==sizeof(int32_t), "unexpected size of int");
// 	return stream << int32_t(v);
// }
// 
// MAssIpcCallDataStream& operator>>(MAssIpcCallDataStream& stream, int& v)
// {
// 	STATIC_ASSERT(sizeof(v)==sizeof(int32_t), "unexpected size of int");
// 	return stream >> reinterpret_cast<int32_t&>(v);
// }

//-------------------------------------------------------


MAssIpcCallDataStream& operator<<(MAssIpcCallDataStream& stream, const std::string& v)
{
	stream<<(uint32_t)(v.length()*sizeof(char));
	stream.WriteRawData(reinterpret_cast<const uint8_t*>(v.c_str()), (uint32_t)(v.length()*sizeof(char)));
	return stream;
}

MAssIpcCallDataStream& operator>>(MAssIpcCallDataStream& stream, std::string& v)
{
	uint32_t length = 0;
	stream>>length;
	v.resize(length/sizeof(char));
	char* raw_data = &v[0];
	if( length!=0 )
		stream.ReadRawData(reinterpret_cast<uint8_t*>(raw_data), length/sizeof(char));
	return stream;
}

//-------------------------------------------------------

MAssIpcCallDataStream& operator<<(MAssIpcCallDataStream& stream, const MAssIpcCall_EnumerateData& v)
{
	stream<<uint32_t(v.size());
	for( MAssIpcData::TPacketSize i = 0; i<v.size(); i++ )
		stream<<v[i].name<<v[i].return_type<<v[i].params_types<<v[i].comment;
	return stream;
}

MAssIpcCallDataStream& operator>>(MAssIpcCallDataStream& stream, MAssIpcCall_EnumerateData& v)
{
	uint32_t size=0;
	stream>>size;
	v.resize(size);
	for( MAssIpcData::TPacketSize i = 0; i<v.size(); i++ )
		stream>>v[i].name>>v[i].return_type>>v[i].params_types>>v[i].comment;
	return stream;
}

//-------------------------------------------------------
