#include "MAssIpc_DataStream.h"
#include <cstring>
#include "MAssIpc_Macros.h"
#include <limits>


MAssIpc_DataStream::MAssIpc_DataStream(std::unique_ptr<const MAssIpc_Data> data_read)
	:m_read(std::move(data_read))
{
}

MAssIpc_DataStream::MAssIpc_DataStream(std::unique_ptr<MAssIpc_Data> data_write)
	:m_write(std::move(data_write))
{
}

MAssIpc_DataStream::~MAssIpc_DataStream()
{
}

bool MAssIpc_DataStream::IsReadBufferPresent()
{
	return bool(m_read);
}

std::unique_ptr<const MAssIpc_Data> MAssIpc_DataStream::DetachRead()
{
	return std::move(m_read);
}

std::unique_ptr<MAssIpc_Data> MAssIpc_DataStream::DetachWrite()
{
	return std::move(m_write);
}

const MAssIpc_Data* MAssIpc_DataStream::GetDataRead() const
{
	return m_read.get();
}

MAssIpc_Data* MAssIpc_DataStream::GetDataWrite() const
{
	return m_write.get();
}

bool MAssIpc_DataStream::CheckAssert(bool assert)
{
	mass_return_x_if_equal(assert, true, assert);
	return assert;
}

void MAssIpc_DataStream::WriteRawData(const uint8_t* data, MAssIpc_Data::TPacketSize len)
{
	uint8_t* bytes = WriteRawData(len);
	if( bytes )
		memcpy(bytes, data, len);
}

uint8_t* MAssIpc_DataStream::WriteRawData(MAssIpc_Data::TPacketSize len)
{
	uint8_t* bytes = nullptr;
	if( m_write )
	{
		MAssIpc_Data::TPacketSize size = m_write->Size();
		mass_return_x_if_equal((m_write_pos<(std::numeric_limits<MAssIpc_Data::TPacketSize>::max()-len))&&(size < m_write_pos+len), true, nullptr);

		bytes = m_write->Data()+m_write_pos;
	}

	m_write_pos += len;// calc necessary size even without storage

	return bytes;
}

const uint8_t* MAssIpc_DataStream::ReadRawData(MAssIpc_Data::TPacketSize len)
{
	mass_return_x_if_equal(IsReadAvailable(len), false, nullptr);
	const uint8_t* pos = m_read->Data()+m_read_pos;
	m_read_pos += len;
	return pos;
}

void MAssIpc_DataStream::ReadRawData(uint8_t* data, MAssIpc_Data::TPacketSize len)
{
	const uint8_t* pos = ReadRawData(len);
	if( pos )
		memcpy(data, pos, len);
	else
		memset(data, 0, len);
}

bool MAssIpc_DataStream::IsReadAvailable(MAssIpc_Data::TPacketSize size)
{
	if( m_read)
	{
		bool res = m_read->Size()-m_read_pos >= size;
		return res;
	}

	return false;
}

MAssIpc_Data::TPacketSize MAssIpc_DataStream::GetWritePos()
{
	return m_write_pos;
}

//-------------------------------------------------------

void MAssIpc_DataStream::ReadRawData(char* data, MAssIpc_Data::TPacketSize len)
{
	uint8_t* u8_data = reinterpret_cast<uint8_t*>(data);
	static_assert(sizeof(*u8_data)==sizeof(*data), "must be same size");
	ReadRawData(u8_data, len);
}

const char* MAssIpc_DataStream::ReadRawDataChar(MAssIpc_Data::TPacketSize len)
{
	const uint8_t* u8_data = ReadRawData(len);
	const char* data = reinterpret_cast<const char*>(u8_data);
	static_assert(sizeof(*u8_data)==sizeof(*data), "must be same size");
	return data;
}

void MAssIpc_DataStream::WriteRawData(const char* data, MAssIpc_Data::TPacketSize len)
{
	const uint8_t* u8_data = reinterpret_cast<const uint8_t*>(data);
	static_assert(sizeof(*u8_data)==sizeof(*data), "must be same size");
	WriteRawData(u8_data, len);
}

MAssIpc_DataStream &MAssIpc_DataStream::operator>>(int8_t &i)
{
	ReadBytes(&i);
	return *this;
}

MAssIpc_DataStream &MAssIpc_DataStream::operator>>(int16_t &i)
{
	ReadBytes(&i);
	return *this;
}

MAssIpc_DataStream &MAssIpc_DataStream::operator>>(int32_t &i)
{
	ReadBytes(&i);
	return *this;
}

MAssIpc_DataStream &MAssIpc_DataStream::operator>>(int64_t &i)
{
	ReadBytes(&i);
	return *this;
}


MAssIpc_DataStream &MAssIpc_DataStream::operator>>(bool &i)
{
	ReadRawData(reinterpret_cast<uint8_t*>(&i), sizeof(i));
	return *this;
}

MAssIpc_DataStream &MAssIpc_DataStream::operator>>(float &f)
{
	ReadRawData(reinterpret_cast<uint8_t*>(&f), sizeof(f));
	return *this;
}

MAssIpc_DataStream &MAssIpc_DataStream::operator>>(double &f)
{
	ReadRawData(reinterpret_cast<uint8_t*>(&f), sizeof(f));
	return *this;
}

MAssIpc_DataStream &MAssIpc_DataStream::operator<<(int8_t i)
{
	WriteBytes(i);
	return *this;
}

MAssIpc_DataStream &MAssIpc_DataStream::operator<<(int16_t i)
{
	WriteBytes(i);
	return *this;
}

MAssIpc_DataStream &MAssIpc_DataStream::operator<<(int32_t i)
{
	WriteBytes(i);
	return *this;
}

MAssIpc_DataStream &MAssIpc_DataStream::operator<<(int64_t i)
{
	WriteBytes(i);
	return *this;
}

MAssIpc_DataStream &MAssIpc_DataStream::operator<<(bool i)
{
	WriteRawData(reinterpret_cast<uint8_t*>(&i), sizeof(i));
	return *this;
}

MAssIpc_DataStream &MAssIpc_DataStream::operator<<(float f)
{
	WriteRawData(reinterpret_cast<uint8_t*>(&f), sizeof(f));
 	return *this;
}

MAssIpc_DataStream &MAssIpc_DataStream::operator<<(double f)
{
	WriteRawData(reinterpret_cast<uint8_t*>(&f), sizeof(f));
	return *this;
}


MAssIpc_DataStream &MAssIpc_DataStream::operator>>(uint8_t &i)
{ 
	return *this >> reinterpret_cast<int8_t&>(i); 
}

MAssIpc_DataStream &MAssIpc_DataStream::operator>>(uint16_t &i)
{ 
	return *this >> reinterpret_cast<int16_t&>(i); 
}

MAssIpc_DataStream &MAssIpc_DataStream::operator>>(uint32_t &i)
{ 
	return *this >> reinterpret_cast<int32_t&>(i); 
}

MAssIpc_DataStream &MAssIpc_DataStream::operator>>(uint64_t &i)
{ 
	return *this >> reinterpret_cast<int64_t&>(i); 
}

MAssIpc_DataStream &MAssIpc_DataStream::operator<<(uint8_t i)
{ 
	return *this << int8_t(i); 
}

MAssIpc_DataStream &MAssIpc_DataStream::operator<<(uint16_t i)
{ 
	return *this << int16_t(i); 
}

MAssIpc_DataStream &MAssIpc_DataStream::operator<<(uint32_t i)
{ 
	return *this << int32_t(i); 
}

MAssIpc_DataStream &MAssIpc_DataStream::operator<<(uint64_t i)
{ 
	return *this << int64_t(i); 
}

//-------------------------------------------------------
// 
// MAssIpc_DataStream& operator<<(MAssIpc_DataStream& stream, const int& v)
// {
// 	STATIC_ASSERT(sizeof(v)==sizeof(int32_t), "unexpected size of int");
// 	return stream << int32_t(v);
// }
// 
// MAssIpc_DataStream& operator>>(MAssIpc_DataStream& stream, int& v)
// {
// 	STATIC_ASSERT(sizeof(v)==sizeof(int32_t), "unexpected size of int");
// 	return stream >> reinterpret_cast<int32_t&>(v);
// }

//-------------------------------------------------------


MAssIpc_DataStream& operator<<(MAssIpc_DataStream& stream, const std::string& v)
{
	stream<<(uint32_t)(v.length()*sizeof(char));
	stream.WriteRawData(reinterpret_cast<const uint8_t*>(v.c_str()), (uint32_t)(v.length()*sizeof(char)));
	return stream;
}

MAssIpc_DataStream& operator>>(MAssIpc_DataStream& stream, std::string& v)
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

MAssIpc_DataStream& operator<<(MAssIpc_DataStream& stream, const MAssIpcCall_EnumerateData& v)
{
	stream<<uint32_t(v.size());
	for( MAssIpc_Data::TPacketSize i = 0; i<v.size(); i++ )
		stream<<v[i].name<<v[i].return_type<<v[i].params_types<<v[i].comment;
	return stream;
}

MAssIpc_DataStream& operator>>(MAssIpc_DataStream& stream, MAssIpcCall_EnumerateData& v)
{
	uint32_t size=0;
	stream>>size;
	v.resize(size);
	for( MAssIpc_Data::TPacketSize i = 0; i<v.size(); i++ )
		stream>>v[i].name>>v[i].return_type>>v[i].params_types>>v[i].comment;
	return stream;
}

//-------------------------------------------------------
