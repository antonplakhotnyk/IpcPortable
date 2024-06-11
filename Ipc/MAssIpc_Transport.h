#pragma once

#include <stdint.h>
#include <memory>
#include "MAssIpc_Data.h"


//-------------------------------------------------------

 
class MAssIpc_TransportShare
{
public:

	virtual ~MAssIpc_TransportShare()=default;

	virtual std::unique_ptr<MAssIpc_Data> Create(MAssIpc_Data::PacketSize size)=0;
	virtual void Write(std::unique_ptr<const MAssIpc_Data> packet)=0;
	// return meaning same as MAssIpc_TransportCopy::WaitRespond
	virtual bool Read(bool wait_incoming_packet, std::unique_ptr<const MAssIpc_Data>* packet)=0;
};

//-------------------------------------------------------

class MAssIpc_TransportCopy
{
protected:
	~MAssIpc_TransportCopy()=default;
public:

	// return false - cancel wait respond
	virtual bool	WaitRespond(size_t expected_size)=0;

	virtual size_t	ReadBytesAvailable()=0;
	virtual bool	Read(uint8_t* data, size_t size)=0;
	virtual void	Write(const uint8_t* data, size_t size)=0;
};

//-------------------------------------------------------
