#include "MAssIpcCallDataStream.h"
#include <cstring>
#include "MAssMacros.h"


static size_t MakeSkip(size_t data_read_only_size, size_t skip)
{
	if( skip > data_read_only_size )
		skip = data_read_only_size;

	return skip;
}

MAssIpcCallDataStream::MAssIpcCallDataStream(std::unique_ptr<MAssIpcData> data_read_write)
	:m_read_write(std::move(data_read_write))
{
}

MAssIpcCallDataStream::~MAssIpcCallDataStream()
{
}

bool MAssIpcCallDataStream::IsDataBufferPresent()
{
	return bool(m_read_write);
}

std::unique_ptr<MAssIpcData> MAssIpcCallDataStream::DetachData()
{
	return std::move(m_read_write);
}

MAssIpcData* MAssIpcCallDataStream::GetData()
{
	return m_read_write.get();
}

template<class T>
void MAssIpcCallDataStream::WriteBytes(T t)
{
	if( m_read_write )
	{
		size_t size = m_read_write->Size();
		mass_return_if_equal(size < m_write_pos+sizeof(t), true);

		uint8_t* bytes = m_read_write->Data()+m_write_pos;
		WriteUnsafe(bytes, t);
	}

	m_write_pos += sizeof(t);
}

template<class T>
void MAssIpcCallDataStream::ReadBytes(T* t)
{
	mass_return_if_equal(IsReadAvailable(sizeof(*t)), false);
	const uint8_t* bytes = m_read_write->Data()+m_read_pos;

	*t = ReadUnsafe<T>(bytes);
	m_read_pos += sizeof(T);
}


void MAssIpcCallDataStream::ReadRawData(uint8_t* data, size_t len)
{
	mass_return_if_equal(IsReadAvailable(len), false);
	const uint8_t* pos = m_read_write->Data()+m_read_pos;
	memcpy(data, pos, len);
	m_read_pos += len;
}

void MAssIpcCallDataStream::WriteRawData(const uint8_t* data, size_t len)
{
	if( m_read_write )
	{
		size_t size = m_read_write->Size();
		mass_return_if_equal(size < m_write_pos+len, true);

		uint8_t* bytes = m_read_write->Data()+m_write_pos;
		memcpy(bytes, data, len);
	}

	m_write_pos += len;
}

bool MAssIpcCallDataStream::IsReadAvailable(size_t size)
{
	if( m_read_write )
	{
		bool res = m_read_write->Size()-m_read_pos >= size;
		return res;
	}

	return false;
}

size_t MAssIpcCallDataStream::GetWritePos()
{
	return m_write_pos;
}

//-------------------------------------------------------

void MAssIpcCallDataStream::ReadRawData(char* data, size_t len)
{
	uint8_t* u8_data = reinterpret_cast<uint8_t*>(data);
	static_assert(sizeof(*u8_data)==sizeof(*data), "must be same size");
	ReadRawData(u8_data, len);
}

void MAssIpcCallDataStream::WriteRawData(const char* data, size_t len)
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
	for( size_t i = 0; i<v.size(); i++ )
		stream<<v[i].name<<v[i].return_type<<v[i].params_types<<v[i].comment;
	return stream;
}

MAssIpcCallDataStream& operator>>(MAssIpcCallDataStream& stream, MAssIpcCall_EnumerateData& v)
{
	uint32_t size=0;
	stream>>size;
	v.resize(size);
	for( size_t i = 0; i<v.size(); i++ )
		stream>>v[i].name>>v[i].return_type>>v[i].params_types>>v[i].comment;
	return stream;
}

//-------------------------------------------------------
