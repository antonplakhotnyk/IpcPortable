#pragma once

#include <stdint.h>

class MAssIpc_Data
{
public:

	typedef uint32_t PacketSize;

	virtual ~MAssIpc_Data()=default;

	virtual PacketSize Size() const=0;
	virtual uint8_t* Data()=0;
	virtual const uint8_t* Data() const=0;
};