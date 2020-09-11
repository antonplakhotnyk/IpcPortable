#include "MAssIpcPacketTransportDefault.h"
#include "MAssMacros.h"
#include "MAssIpcCallInternal.h"

namespace MAssIpcCallInternal
{

MAssIpcPacketTransportDefault::MAssIpcPacketTransportDefault(const std::weak_ptr<MAssIpcCallTransport>& transport)
	:m_transport(transport)
{
}

std::unique_ptr<MAssIpcData> MAssIpcPacketTransportDefault::Create(size_t size)
{
	return std::unique_ptr<MAssIpcData>(new MAssIpcData_Vector(size));
}

void MAssIpcPacketTransportDefault::Write(std::unique_ptr<MAssIpcData> packet)
{
	std::shared_ptr<MAssIpcCallTransport> transport = m_transport.lock();
	mass_return_if_equal(bool(transport), false);
	transport->Write(packet->Data(), packet->Size());
}

bool MAssIpcPacketTransportDefault::Read(bool wait_incoming_packet, std::unique_ptr<MAssIpcData>* packet)
{
	std::shared_ptr<MAssIpcCallTransport> transport = m_transport.lock();
	mass_return_x_if_equal(bool(transport), false, {});

	size_t needed_data_size = 0;
	needed_data_size = m_packet_parser.ReadNeededDataSize(transport, packet);
	if( !bool(*packet) )
	{
		if( wait_incoming_packet )
		{
			bool transport_ok = transport->WaitRespond(needed_data_size);
			if( transport_ok )
				m_packet_parser.ReadNeededDataSize(transport, packet);
			return transport_ok;
		}
	}

	return true;
}

}