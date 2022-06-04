#pragma once

#include "MAssIpcCallDataStream.h"
#include <cstring>

namespace MAssIpcCallInternal
{

class MAssIpcRawString
{
public:

	template<size_t N>
	constexpr MAssIpcRawString(const char(&str)[N])
		:m_str(str)
		, m_len(N-1)
	{
		static_assert(N>=1, "unexpected string size");
	}

	template<class T, typename = typename std::enable_if<!std::is_array<T>::value>::type>
	MAssIpcRawString(const T str)
		:MAssIpcRawString(str, ConvertCheckStrLen(strlen(str)))
	{
	}

	MAssIpcRawString(const std::string& str)
		:MAssIpcRawString(str.data(), ConvertCheckStrLen(str.length()))
	{
	}

	constexpr MAssIpcRawString(const char* str, MAssIpcData::TPacketSize len)
		: m_str(str)
		, m_len(len)
	{
	}

	constexpr MAssIpcRawString(const MAssIpcRawString& other) = default;

	static MAssIpcRawString Read(MAssIpcCallDataStream& stream);
	void Write(MAssIpcCallDataStream& stream) const;

	constexpr const char* C_String() const
	{
		return m_str;
	}

	inline std::string Std_String() const
	{
		return {m_str, m_len};
	}

	constexpr MAssIpcData::TPacketSize Length() const
	{
		return m_len;
	}

	inline bool operator== (const MAssIpcRawString& other) const
	{
		if( m_len != other.m_len )
			return false;

		return memcmp(m_str, other.m_str, m_len)==0;
	}

	inline bool operator!= (const MAssIpcRawString& other) const
	{
		return !(operator==)(other);
	}

	inline bool operator< (const MAssIpcRawString& other) const
	{
		return std::lexicographical_compare(m_str, m_str+m_len, other.m_str, other.m_str+other.m_len);
	}

private:

	static MAssIpcData::TPacketSize ConvertCheckStrLen(size_t str_len);

private:

	const char* const m_str;
	const MAssIpcData::TPacketSize m_len;
};

}