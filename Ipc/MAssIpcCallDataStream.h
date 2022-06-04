#pragma once

#include <stdint.h>
#include <string>
#include "MAssIpcCallTransport.h"
#include <vector>

template<class TType>
struct MAssIpcType
{
	static_assert(sizeof(TType)!=sizeof(TType), "custom MAssIpcType must be defined for type TType use MASS_IPC_TYPE_SIGNATURE");
	static constexpr const char name_value[1] = "";
};

//-------------------------------------------------------


// name_value used for compile time checks						  	
// NameValue used in runtime									  	
// in c++17 name_value can be used in compile and runtime 		  	
// as soon as all template statics are inlined					  	
// before c++17 name_value declaration necessary 				  	
// in single cpp file for each specialization					  	
#define MASS_IPC_TYPE_SIGNATURE(type)									\
template<>															  	\
struct MAssIpcType<type>												\
{																		\
	static constexpr const char name_value[sizeof(#type)] = #type;		\
	static constexpr MAssIpcData::TPacketSize NameLength()								\
	{																	\
		return std::extent<decltype(name_value)>::value-1;				\
	}																	\
	inline static constexpr const char* NameValue()						\
	{																	\
		return #type;													\
	}																	\
};																		

//-------------------------------------------------------

class MAssIpcCallDataStream
{
public:

	MAssIpcCallDataStream(MAssIpcCallDataStream&&) = default;
	MAssIpcCallDataStream(std::unique_ptr<const MAssIpcData> data_read);
	MAssIpcCallDataStream(std::unique_ptr<MAssIpcData> data_write);
	MAssIpcCallDataStream() = default;
	MAssIpcCallDataStream& operator= (MAssIpcCallDataStream&&) = default;
	~MAssIpcCallDataStream();

	MAssIpcCallDataStream &operator>>(int8_t &i);
	MAssIpcCallDataStream &operator>>(uint8_t &i);
	MAssIpcCallDataStream &operator>>(int16_t &i);
	MAssIpcCallDataStream &operator>>(uint16_t &i);
	MAssIpcCallDataStream &operator>>(int32_t &i);
	MAssIpcCallDataStream &operator>>(uint32_t &i);
	MAssIpcCallDataStream &operator>>(int64_t &i);
	MAssIpcCallDataStream &operator>>(uint64_t &i);

	MAssIpcCallDataStream &operator>>(bool &i);
	MAssIpcCallDataStream &operator>>(float &f);
	MAssIpcCallDataStream &operator>>(double &f);

	MAssIpcCallDataStream &operator<<(int8_t i);
	MAssIpcCallDataStream &operator<<(uint8_t i);
	MAssIpcCallDataStream &operator<<(int16_t i);
	MAssIpcCallDataStream &operator<<(uint16_t i);
	MAssIpcCallDataStream &operator<<(int32_t i);
	MAssIpcCallDataStream &operator<<(uint32_t i);
	MAssIpcCallDataStream &operator<<(int64_t i);
	MAssIpcCallDataStream &operator<<(uint64_t i);
	MAssIpcCallDataStream &operator<<(bool i);
	MAssIpcCallDataStream &operator<<(float f);
	MAssIpcCallDataStream &operator<<(double f);

 	MAssIpcCallDataStream& operator<<(void* p)=delete;// protect for implicit conversion of pointer to bool

	void ReadRawData(uint8_t* data, MAssIpcData::TPacketSize len);
	void ReadRawData(char* data, MAssIpcData::TPacketSize len);
	const uint8_t* ReadRawData(MAssIpcData::TPacketSize len);
	const char* ReadRawDataChar(MAssIpcData::TPacketSize len);
	void WriteRawData(const uint8_t* data, MAssIpcData::TPacketSize len);
	void WriteRawData(const char* data, MAssIpcData::TPacketSize len);
	uint8_t* WriteRawData(MAssIpcData::TPacketSize len);

	std::unique_ptr<const MAssIpcData> DetachRead();
	std::unique_ptr<MAssIpcData> DetachWrite();
	const MAssIpcData* GetDataRead() const;
	MAssIpcData* GetDataWrite() const;
	MAssIpcData::TPacketSize GetWritePos();
	bool IsReadBufferPresent();

private:

	static bool CheckAssert(bool assert);

public:

	bool IsReadAvailable(MAssIpcData::TPacketSize size);

	template<class T>
	void WriteBytes(T t)
	{
		if( m_write )
		{
			MAssIpcData::TPacketSize size = m_write->Size();
			if( CheckAssert((m_write_pos<(std::numeric_limits<MAssIpcData::TPacketSize>::max()-sizeof(t)))&&(size < m_write_pos+sizeof(t))) )
				return;

			uint8_t* bytes = m_write->Data()+m_write_pos;
			WriteUnsafe(bytes, t);
		}

		m_write_pos += sizeof(t);
	}

	template<class T>
	void ReadBytes(T* t)
	{
		if( CheckAssert(!IsReadAvailable(sizeof(*t))) )
			return;
		const uint8_t* bytes = m_read->Data()+m_read_pos;

		*t = ReadUnsafe<T>(bytes);
		m_read_pos += sizeof(T);
	}

	template<class T>
	static void WriteUnsafe(uint8_t* bytes, T t)
	{
		if( sizeof(T)==1 )
			bytes[0] = uint8_t(t);
		else
		{
			for( MAssIpcData::TPacketSize i = 0; i<sizeof(T); i++ )
			{
				bytes[i] = (0xFF & t);
				t >>= 8;
			}
		}
	}

	template<class T>
	static T ReadUnsafe(const uint8_t* bytes)
	{
		if( sizeof(T)==1 )
			return T(bytes[0]);
		else
		{
			T t = 0;
			for( MAssIpcData::TPacketSize i = 0; i<sizeof(T); i++ )
			{
				t <<= 8;
				t |= bytes[sizeof(T)-1-i];
			}
			return t;
		}
	}


private:

	std::unique_ptr<const MAssIpcData> m_read;
	std::unique_ptr<MAssIpcData> m_write;

	MAssIpcData::TPacketSize	m_read_pos = 0;
	MAssIpcData::TPacketSize	m_write_pos = 0;
};

//-------------------------------------------------------

struct MAssIpcCall_ProcDescription
{
	std::string name;
	std::string return_type;
	std::string params_types;
	std::string comment;
};

typedef std::vector<MAssIpcCall_ProcDescription> MAssIpcCall_EnumerateData;

MAssIpcCallDataStream& operator<<(MAssIpcCallDataStream& stream, const MAssIpcCall_EnumerateData& v);
MAssIpcCallDataStream& operator>>(MAssIpcCallDataStream& stream, MAssIpcCall_EnumerateData& v);
MASS_IPC_TYPE_SIGNATURE(MAssIpcCall_EnumerateData);

//-------------------------------------------------------


// MAssIpcCallDataStream& operator<<(MAssIpcCallDataStream& stream, const int& v);
// MAssIpcCallDataStream& operator>>(MAssIpcCallDataStream& stream, int& v);

//-------------------------------------------------------

MAssIpcCallDataStream& operator<<(MAssIpcCallDataStream& stream, const std::string& v);
MAssIpcCallDataStream& operator>>(MAssIpcCallDataStream& stream, std::string& v);

MASS_IPC_TYPE_SIGNATURE(std::string);

// template<class TType> 
// struct MAssIpcType; 
// template<> struct MAssIpcType<std::string> { static const char* Name() { return "std::string"; }; };;

//-------------------------------------------------------

template<typename Integer, std::enable_if_t<std::is_integral<Integer>::value, bool> = true>
MAssIpcCallDataStream& operator<<(MAssIpcCallDataStream& stream, const std::vector<Integer>& v)
{
	stream<<(uint32_t)(v.size()*sizeof(Integer));
	for( const Integer& item : v )
		stream.WriteBytes(item);
	return stream;
}

template<typename Integer, std::enable_if_t<std::is_integral<Integer>::value, bool> = true>
MAssIpcCallDataStream& operator>>(MAssIpcCallDataStream& stream, std::vector<Integer>& v)
{
	uint32_t length = 0;
	stream>>length;
	v.resize(length/sizeof(Integer));
	for( Integer& item : v )
		stream.ReadBytes(&item);
	return stream;
}

MASS_IPC_TYPE_SIGNATURE(std::vector<uint8_t>);
MASS_IPC_TYPE_SIGNATURE(std::vector<uint16_t>);
MASS_IPC_TYPE_SIGNATURE(std::vector<uint32_t>);
MASS_IPC_TYPE_SIGNATURE(std::vector<uint64_t>);
MASS_IPC_TYPE_SIGNATURE(std::vector<int8_t>);
MASS_IPC_TYPE_SIGNATURE(std::vector<int16_t>);
MASS_IPC_TYPE_SIGNATURE(std::vector<int32_t>);
MASS_IPC_TYPE_SIGNATURE(std::vector<int64_t>);

//-------------------------------------------------------

