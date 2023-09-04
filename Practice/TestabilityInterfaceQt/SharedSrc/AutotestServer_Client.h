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


	// Must not exist. Use regular wait-event mechanism
	struct Handlers
	{
		std::function<SutIndexId()> OnConnectedSut;
		std::function<void()> OnDisconnectedSut;
	};

	AutotestServer_Client(const Handlers& handlers)
		: m_int{std::make_shared<Internals>(Internals{{}, handlers})}
	{
	}

	const MAssIpcCall& IpcCall(SutIndexId sut_index_id = {}) const
	{
		if( sut_index_id >=SutIndexId::max_count )
			return m_disconnected_stub;
		if( !bool(m_int->m_client_connections[size_t(sut_index_id)]) )
			return m_disconnected_stub;

		return m_int->m_client_connections[size_t(sut_index_id)]->ipc_net.ipc_call;
	};

private:

	MAssIpcCall		m_disconnected_stub{std::weak_ptr<MAssIpc_Transthread>{}};

protected:

	struct Internals
	{
		std::shared_ptr<SutConnection> m_client_connections[size_t(SutIndexId::max_count)];
		const Handlers		m_handlers;
	};

	std::shared_ptr<Internals> m_int;

public:

	class Event
	{
	public:
		Event(std::weak_ptr<Internals> internals)
			:m_int{internals}
		{
		}

		enum struct LocalId
		{
			ConnectedSut,
			DisconnectedSut,
		};

		void BindLocal(LocalId event_id)
		{
		}

		void BindSut(std::shared_ptr<const MAssIpcCall::CallInfo> event_id, SutIndexId sut_index_id)
		{
		}

		void Wait()
		{
		}

	private:
		const std::weak_ptr<Internals> m_int;
	};

	std::unique_ptr<Event> CreateEvent()
	{
		return std::unique_ptr<Event>{new Event(m_int)};
	}

};
