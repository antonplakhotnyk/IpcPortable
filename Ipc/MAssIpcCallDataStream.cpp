#include "MAssIpcCallDataStream.h"
#include <cstring>
#include "Integration/MAssMacros.h"


MAssIpcCallDataStream::MAssIpcCallDataStream(const uint8_t* data_read_only, size_t data_size)
	:m_pos(0)
	, m_data_read_write(NULL)
	, m_data_read_only(data_read_only)
	, m_data_read_only_size(data_size)
{
}

MAssIpcCallDataStream::MAssIpcCallDataStream(std::vector<uint8_t>* data_read_write)
	: m_pos(0)
	, m_data_read_write(data_read_write)
	, m_data_read_only(NULL)
	, m_data_read_only_size(0)
{
}

MAssIpcCallDataStream::~MAssIpcCallDataStream()
{
}

template<class T>
void MAssIpcCallDataStream::WriteBytes(T t)
{
	mass_return_if_equal(m_data_read_write,NULL);

	size_t pos = Size();
	ResizeRW(pos+sizeof(T));
	mass_return_if_not_equal(Size(), pos+sizeof(t));

	uint8_t* bytes = DataRW()+pos;

	for( size_t i = 0; i<sizeof(T); i++ )
	{
		bytes[i] = (0xFF & t);
		t >>= 8;
	}
}

template<class T>
void MAssIpcCallDataStream::ReadBytes(T* t)
{
	mass_return_if_equal(IsReadAvailable(sizeof(*t)), false);
	const uint8_t* bytes = DataR()+m_pos;

	*t = 0;
	for( size_t i = 0; i<sizeof(T); i++ )
	{
		*t <<= 8;
		*t |= bytes[sizeof(T)-1-i];
	}

	m_pos += sizeof(T);
}


void MAssIpcCallDataStream::ReadRawData(uint8_t* data, size_t len)
{
	mass_return_if_equal(IsReadAvailable(len), false);
	const uint8_t* pos = DataR()+m_pos;
	memcpy(data, pos, len);
	m_pos += len;
}

void MAssIpcCallDataStream::WriteRawData(const uint8_t* data, size_t len)
{
	mass_return_if_equal(m_data_read_write, NULL);

	size_t pos = Size();
	ResizeRW(pos+len);
	mass_return_if_not_equal(Size(), pos+len);

	uint8_t* bytes = DataRW()+pos;
	memcpy(bytes, data, len);
}

bool MAssIpcCallDataStream::IsReadAvailable(size_t size)
{
	bool res = Size()-m_pos >= size;
	return res;
}

//-------------------------------------------------------

uint8_t* MAssIpcCallDataStream::DataRW()
{
	return &(*m_data_read_write)[0];
}

size_t MAssIpcCallDataStream::Size()
{
	if( m_data_read_write )
		return m_data_read_write->size();
	return m_data_read_only_size;
}

void MAssIpcCallDataStream::ResizeRW(size_t size)
{
	m_data_read_write->resize(size);
}

const uint8_t* MAssIpcCallDataStream::DataR() const
{
	if( m_data_read_write )
		return &(*m_data_read_write)[0];
	return m_data_read_only;
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
