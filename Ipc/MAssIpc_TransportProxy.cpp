#include "MAssIpc_TransportProxy.h"
#include "MAssIpc_Macros.h"
#include "MAssIpc_Internals.h "

namespace MAssIpcCallInternal
{

TransportProxy::TransportProxy(const std::weak_ptr<MAssIpc_TransportCopy>& transport)
	:m_transport(transport)
{
}

std::unique_ptr<MAssIpc_Data> TransportProxy::Create(MAssIpc_Data::TPacketSize size)
{
	return std::unique_ptr<MAssIpc_Data>(std::make_unique<MAssIpcData_Vector>(size));
}

void TransportProxy::Write(std::unique_ptr<const MAssIpc_Data> packet)
{
	std::shared_ptr<MAssIpc_TransportCopy> transport = m_transport.lock();
	mass_return_if_equal(bool(transport), false);
	transport->Write(packet->Data(), packet->Size());
}

bool TransportProxy::Read(bool wait_incoming_packet, std::unique_ptr<const MAssIpc_Data>* packet)
{
	std::shared_ptr<MAssIpc_TransportCopy> transport = m_transport.lock();
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