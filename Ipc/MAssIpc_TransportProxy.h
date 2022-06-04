#pragma once

#include <vector>
#include "MAssIpc_Transport.h"
#include "MAssIpc_PacketParser.h"

namespace MAssIpcCallInternal
{

class TransportProxy: public MAssIpc_TransportShare
{
public:

	TransportProxy(const std::weak_ptr<MAssIpc_TransportCopy>& transport);

	std::unique_ptr<MAssIpc_Data> Create(MAssIpc_Data::TPacketSize size) override;
	void Write(std::unique_ptr<const MAssIpc_Data> packet) override;
	bool Read(bool wait_incoming_packet, std::unique_ptr<const MAssIpc_Data>* packet) override;

private:

private:

	MAssIpcCallInternal::MAssIpc_PacketParser		m_packet_parser;
	const std::weak_ptr<MAssIpc_TransportCopy>		m_transport;
};

}