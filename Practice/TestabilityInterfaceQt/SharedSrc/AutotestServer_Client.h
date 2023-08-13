#pragma once

#include "IpcQt_Net.h"

class AutotestServer_Client
{
public:

	struct SutConnection
	{
		SutConnection(MAssIpcCall& ipc_connection, std::shared_ptr<IpcQt_TransporTcp> transport)
			: ipc_net(ipc_connection, transport)
		{
			this->ipc_net.ipc_call.SetErrorHandler(&SutConnection::IpcError);
		}

		IpcQt_Net		ipc_net;

	private:
		static void IpcError(MAssIpcCall::ErrorType et, std::string message);
	};

public:

	enum struct SutIndexId:size_t
	{
		on_connected_sut = 16,
		max_count = on_connected_sut+1,
	};


	struct Handlers
	{
		std::function<SutIndexId()> OnConnectedSut;
		std::function<void()> OnDisconnectedSut;
	};

	AutotestServer_Client(const Handlers& handlers)
		: m_handlers(handlers)
	{
	}

	const MAssIpcCall& IpcCall(SutIndexId sut_index_id = {}) const
	{
		if( sut_index_id >=SutIndexId::max_count )
			return m_disconnected_stub;
		if( !bool(m_client_connections[size_t(sut_index_id)]) )
			return m_disconnected_stub;

		return m_client_connections[size_t(sut_index_id)]->ipc_net.ipc_call;
	};

private:

	MAssIpcCall		m_disconnected_stub{std::weak_ptr<MAssIpc_Transthread>{}};

protected:

	std::shared_ptr<SutConnection> m_client_connections[size_t(SutIndexId::max_count)];
	Handlers		m_handlers;
};
