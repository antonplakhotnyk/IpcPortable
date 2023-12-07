#pragma once

#include <stdint.h>
#include "MAssIpc_Packet.h"

class MAssIpc_Data
{
public:

	using PacketSize=MAssIpcImpl::MAssIpc_Packet::PacketSize;

	virtual ~MAssIpc_Data()=default;

	virtual PacketSize Size() const=0;
	virtual uint8_t* Data()=0;
	virtual const uint8_t* Data() const=0;
};