#pragma once

#include <stdint.h>
#include <string>
#include "MAssIpc_Transport.h"
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
	static constexpr MAssIpc_Data::TPacketSize NameLength()								\
	{																	\
		return std::extent<decltype(name_value)>::value-1;				\
	}																	\
	inline static constexpr const char* NameValue()						\
	{																	\
		return #type;													\
	}																	\
};																		

//-------------------------------------------------------

class MAssIpc_DataStream
{
public:

	MAssIpc_DataStream(MAssIpc_DataStream&&) = default;
	MAssIpc_DataStream(std::unique_ptr<const MAssIpc_Data> data_read);
	MAssIpc_DataStream(std::unique_ptr<MAssIpc_Data> data_write);
	MAssIpc_DataStream() = default;
	MAssIpc_DataStream& operator= (MAssIpc_DataStream&&) = default;
	~MAssIpc_DataStream();

	MAssIpc_DataStream &operator>>(int8_t &i);
	MAssIpc_DataStream &operator>>(uint8_t &i);
	MAssIpc_DataStream &operator>>(int16_t &i);
	MAssIpc_DataStream &operator>>(uint16_t &i);
	MAssIpc_DataStream &operator>>(int32_t &i);
	MAssIpc_DataStream &operator>>(uint32_t &i);
	MAssIpc_DataStream &operator>>(int64_t &i);
	MAssIpc_DataStream &operator>>(uint64_t &i);

	MAssIpc_DataStream &operator>>(bool &i);
	MAssIpc_DataStream &operator>>(float &f);
	MAssIpc_DataStream &operator>>(double &f);

	MAssIpc_DataStream &operator<<(int8_t i);
	MAssIpc_DataStream &operator<<(uint8_t i);
	MAssIpc_DataStream &operator<<(int16_t i);
	MAssIpc_DataStream &operator<<(uint16_t i);
	MAssIpc_DataStream &operator<<(int32_t i);
	MAssIpc_DataStream &operator<<(uint32_t i);
	MAssIpc_DataStream &operator<<(int64_t i);
	MAssIpc_DataStream &operator<<(uint64_t i);
	MAssIpc_DataStream &operator<<(bool i);
	MAssIpc_DataStream &operator<<(float f);
	MAssIpc_DataStream &operator<<(double f);

 	MAssIpc_DataStream& operator<<(void* p)=delete;// protect for implicit conversion of pointer to bool

	void ReadRawData(uint8_t* data, MAssIpc_Data::TPacketSize len);
	void ReadRawData(char* data, MAssIpc_Data::TPacketSize len);
	const uint8_t* ReadRawData(MAssIpc_Data::TPacketSize len);
	const char* ReadRawDataChar(MAssIpc_Data::TPacketSize len);
	void WriteRawData(const uint8_t* data, MAssIpc_Data::TPacketSize len);
	void WriteRawData(const char* data, MAssIpc_Data::TPacketSize len);
	uint8_t* WriteRawData(MAssIpc_Data::TPacketSize len);

	std::unique_ptr<const MAssIpc_Data> DetachRead();
	std::unique_ptr<MAssIpc_Data> DetachWrite();
	const MAssIpc_Data* GetDataRead() const;
	MAssIpc_Data* GetDataWrite() const;
	MAssIpc_Data::TPacketSize GetWritePos();
	bool IsReadBufferPresent();

private:

	static bool CheckAssert(bool assert);

public:

	bool IsReadAvailable(MAssIpc_Data::TPacketSize size);

	template<class T>
	void WriteBytes(T t)
	{
		if( m_write )
		{
			MAssIpc_Data::TPacketSize size = m_write->Size();
			if( CheckAssert((m_write_pos<(std::numeric_limits<MAssIpc_Data::TPacketSize>::max()-sizeof(t)))&&(size < m_write_pos+sizeof(t))) )
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
			for( MAssIpc_Data::TPacketSize i = 0; i<sizeof(T); i++ )
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
			for( MAssIpc_Data::TPacketSize i = 0; i<sizeof(T); i++ )
			{
				t <<= 8;
				t |= bytes[sizeof(T)-1-i];
			}
			return t;
		}
	}


private:

	std::unique_ptr<const MAssIpc_Data> m_read;
	std::unique_ptr<MAssIpc_Data> m_write;

	MAssIpc_Data::TPacketSize	m_read_pos = 0;
	MAssIpc_Data::TPacketSize	m_write_pos = 0;
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

MAssIpc_DataStream& operator<<(MAssIpc_DataStream& stream, const MAssIpcCall_EnumerateData& v);
MAssIpc_DataStream& operator>>(MAssIpc_DataStream& stream, MAssIpcCall_EnumerateData& v);
MASS_IPC_TYPE_SIGNATURE(MAssIpcCall_EnumerateData);

//-------------------------------------------------------


// MAssIpc_DataStream& operator<<(MAssIpc_DataStream& stream, const int& v);
// MAssIpc_DataStream& operator>>(MAssIpc_DataStream& stream, int& v);

//-------------------------------------------------------

MAssIpc_DataStream& operator<<(MAssIpc_DataStream& stream, const std::string& v);
MAssIpc_DataStream& operator>>(MAssIpc_DataStream& stream, std::string& v);

MASS_IPC_TYPE_SIGNATURE(std::string);

// template<class TType> 
// struct MAssIpcType; 
// template<> struct MAssIpcType<std::string> { static const char* Name() { return "std::string"; }; };;

//-------------------------------------------------------

template<typename Integer, std::enable_if_t<std::is_integral<Integer>::value, bool> = true>
MAssIpc_DataStream& operator<<(MAssIpc_DataStream& stream, const std::vector<Integer>& v)
{
	stream<<(uint32_t)(v.size()*sizeof(Integer));
	for( const Integer& item : v )
		stream.WriteBytes(item);
	return stream;
}

template<typename Integer, std::enable_if_t<std::is_integral<Integer>::value, bool> = true>
MAssIpc_DataStream& operator>>(MAssIpc_DataStream& stream, std::vector<Integer>& v)
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

