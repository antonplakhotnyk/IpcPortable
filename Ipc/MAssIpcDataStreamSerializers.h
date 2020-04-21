#pragma once

#include "MAssIpcCallDataStream.h"
#include "ValueUndefined.h"

template<class TVal>
MAssIpcCallDataStream& operator<<(MAssIpcCallDataStream& stream, const ValueUndefined<TVal>& v);
template<class TVal>
MAssIpcCallDataStream& operator>>(MAssIpcCallDataStream& stream, ValueUndefined<TVal>& v);

MASS_IPC_TYPE_SIGNATURE(ValueUndefined<int8_t>);
MASS_IPC_TYPE_SIGNATURE(ValueUndefined<uint8_t>);
MASS_IPC_TYPE_SIGNATURE(ValueUndefined<int16_t>);
MASS_IPC_TYPE_SIGNATURE(ValueUndefined<uint16_t>);
MASS_IPC_TYPE_SIGNATURE(ValueUndefined<int32_t>);
MASS_IPC_TYPE_SIGNATURE(ValueUndefined<uint32_t>);
MASS_IPC_TYPE_SIGNATURE(ValueUndefined<int64_t>);
MASS_IPC_TYPE_SIGNATURE(ValueUndefined<uint64_t>);
MASS_IPC_TYPE_SIGNATURE(ValueUndefined<bool>);
MASS_IPC_TYPE_SIGNATURE(ValueUndefined<float>);
MASS_IPC_TYPE_SIGNATURE(ValueUndefined<double>);

//-------------------------------------------------------

template<class TVal>
MAssIpcCallDataStream& operator<<(MAssIpcCallDataStream& stream, const ValueUndefined<TVal>& v)
{
	stream<<v.IsDefined();
	if( v.IsDefined() )
		stream<<v.Val();
	return stream;
}

template<class TVal>
MAssIpcCallDataStream& operator>>(MAssIpcCallDataStream& stream, ValueUndefined<TVal>& v)
{
	bool is_defined;
	stream>>is_defined;
	if( is_defined )
	{
		TVal val;
		stream>>val;
		v = val;
	}
	return stream;
}

