#pragma once

#include <cstddef>
#include <vector>
#include <string>

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
// as soon as all template statics ar inlined					  	
// before c++17 name_value declaration necessary 				  	
// in single cpp file for each specialization					  	
#define MASS_IPC_TYPE_SIGNATURE(type)									\
template<>															  	\
struct MAssIpcType<type>												\
{																		\
	static constexpr const char name_value[sizeof(#type)] = #type;		\
	inline static constexpr const char* NameValue()						\
	{																	\
		return #type;													\
	}																	\
};																		

//-------------------------------------------------------

class MAssIpcCallDataStream
{
public:

	MAssIpcCallDataStream(const uint8_t* data_read_only, size_t data_size);
	MAssIpcCallDataStream(std::vector<uint8_t>* data_read_write);
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

	void ReadRawData(uint8_t* data, size_t len);
	void ReadRawData(char* data, size_t len);
	void WriteRawData(const uint8_t* data, size_t len);
	void WriteRawData(const char* data, size_t len);

	int SkipRawData(size_t len);

public:

	bool IsReadAvailable(size_t size);

	template<class T>
	void WriteBytes(T t);

	template<class T>
	void ReadBytes(T* t);

	uint8_t* DataRW();
	size_t Size();
	void ResizeRW(size_t size);
	const uint8_t* DataR() const;

private:

	size_t	m_pos;

	std::vector<uint8_t>* const m_data_read_write;

	const uint8_t* const m_data_read_only;
	const size_t m_data_read_only_size;
};

//-------------------------------------------------------

// MAssIpcCallDataStream& operator<<(MAssIpcCallDataStream& stream, const int& v);
// MAssIpcCallDataStream& operator>>(MAssIpcCallDataStream& stream, int& v);

//-------------------------------------------------------

MAssIpcCallDataStream& operator<<(MAssIpcCallDataStream& stream, const std::string& v);
MAssIpcCallDataStream& operator>>(MAssIpcCallDataStream& stream, std::string& v);

MASS_IPC_TYPE_SIGNATURE(std::string);
 
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

// template<class TType> 
// struct MAssIpcType; 
// template<> struct MAssIpcType<std::string> { static const char* Name() { return "std::string"; }; };;

//-------------------------------------------------------

