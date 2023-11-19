#pragma once

#include "MAssIpc_DataStream.h"
#include <cstring>

namespace MAssIpcImpl
{

class RawString
{
public:

	template<size_t N>
	constexpr RawString(const char(&str)[N])
		:m_str(str)
		, m_len(N-1)
	{
		static_assert(N>=1, "unexpected string size");
	}

	template<typename T, typename std::enable_if<!std::is_array<T>::value && std::is_convertible<T, const char*>::value, bool>::type = true>
	RawString(const T str)
		:RawString(str, ConvertCheckStrLen(strlen(str)))
	{
	}

	RawString(const std::string& str)
		:RawString(str.data(), ConvertCheckStrLen(str.length()))
	{
	}

	constexpr RawString(const char* str, MAssIpc_Data::PacketSize len)
		: m_str(str)
		, m_len(len)
	{
	}

	constexpr RawString(const RawString& other)=default;

	static RawString Read(MAssIpc_DataStream& stream);
	void Write(MAssIpc_DataStream& stream) const;

	constexpr const char* C_String() const
	{
		return m_str;
	}

	inline std::string Std_String() const
	{
		return {m_str, m_len};
	}

	constexpr MAssIpc_Data::PacketSize Length() const
	{
		return m_len;
	}

	inline bool operator== (const RawString& other) const
	{
		if( m_len != other.m_len )
			return false;

		return memcmp(m_str, other.m_str, m_len)==0;
	}

	inline bool operator!= (const RawString& other) const
	{
		return !(operator==)(other);
	}

	inline bool operator< (const RawString& other) const
	{
		return std::lexicographical_compare(m_str, m_str+m_len, other.m_str, other.m_str+other.m_len);
	}

private:

	static MAssIpc_Data::PacketSize ConvertCheckStrLen(size_t str_len);

private:

	const char* const m_str;
	const MAssIpc_Data::PacketSize m_len;
};

}