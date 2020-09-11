#pragma once

#include <vector>
#include "MAssIpcCallTransport.h"
#include "MAssIpcPacketParser.h"

namespace MAssIpcCallInternal
{

class MAssIpcPacketTransportDefault: public MAssIpcPacketTransport
{
public:

	MAssIpcPacketTransportDefault(const std::weak_ptr<MAssIpcCallTransport>& transport);

	std::unique_ptr<MAssIpcData> Create(MAssIpcData::TPacketSize size) override;
	void Write(std::unique_ptr<MAssIpcData> packet) override;
	bool Read(bool wait_incoming_packet, std::unique_ptr<MAssIpcData>* packet) override;

private:

private:

	MAssIpcCallInternal::MAssIpcPacketParser		m_packet_parser;
	const std::weak_ptr<MAssIpcCallTransport>		m_transport;
};

}