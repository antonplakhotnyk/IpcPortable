#pragma once

#include <stdint.h>
#include <memory>


//-------------------------------------------------------

class MAssIpcData
{
public:

	typedef uint32_t TPacketSize;

	virtual TPacketSize Size() const = 0;
	virtual uint8_t* Data() = 0;
};

 
class MAssIpcPacketTransport
{
public:

	virtual std::unique_ptr<MAssIpcData> Create(MAssIpcData::TPacketSize size) = 0;
	virtual void Write(std::unique_ptr<MAssIpcData> packet) = 0;
	virtual bool Read(bool wait_incoming_packet, std::unique_ptr<MAssIpcData>* packet) = 0;
};

//-------------------------------------------------------

class MAssIpcCallTransport
{
protected:
	~MAssIpcCallTransport() = default;
public:

	// return false - cancel wait respond
	virtual bool	WaitRespond(size_t expected_size) = 0;

	virtual size_t	ReadBytesAvailable() = 0;
	virtual void	Read(uint8_t* data, size_t size) = 0;
	virtual void	Write(const uint8_t* data, size_t size) = 0;
};

//-------------------------------------------------------
