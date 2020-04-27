#pragma once

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
