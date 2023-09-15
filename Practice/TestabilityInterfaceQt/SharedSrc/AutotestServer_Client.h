#pragma once

#include "IpcQt_Net.h"

class AutotestServerInternals
{
public:

	void IpcCallCountChanged(const std::shared_ptr<const MAssIpcCall::CallInfo>& call_info)
	{
	}

private:

};

//-------------------------------------------------------

class AutotestServer_Client
{
protected:

	struct Internals;

public:

	struct SutConnection
	{
		SutConnection(MAssIpcCall& ipc_connection, std::shared_ptr<IpcQt_TransporTcp> transport, std::weak_ptr<AutotestServerInternals> internals)
			: ipc_net(ipc_connection, transport)
			, m_internals{internals}
		{
			this->ipc_net.ipc_call.SetHandler_ErrorOccured(&SutConnection::IpcError, this);
			this->ipc_net.ipc_call.SetHandler_CallCountChanged([this](const std::shared_ptr<const MAssIpcCall::CallInfo>& call_info)
			{
				if( auto client_int = m_internals.lock() )
					client_int->IpcCallCountChanged(call_info);
			}, this);
		}

		~SutConnection()
		{
			this->ipc_net.ipc_call.ClearHandlersWithTag(this);
		}

		IpcQt_Net		ipc_net;

	private:
		static	void IpcError(MAssIpcCall::ErrorType et, const std::string& message);

	private:
		const std::weak_ptr<AutotestServerInternals> m_internals;
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
		: m_handlers{handlers}
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

	struct Waiter
	{
	};

public:

	class Event
	{
	public:
		Event(std::weak_ptr<AutotestServerInternals> internals)
			:m_internals{internals}
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

		void BindSut(std::shared_ptr<const MAssIpcCall::CallInfo> event_id)
		{
		}

		void Wait()
		{
		}

	private:
		const std::weak_ptr<AutotestServerInternals> m_internals;
	};

	std::unique_ptr<Event> CreateEvent()
	{
		return std::unique_ptr<Event>{new Event(m_server_internals)};
	}

protected:

	std::shared_ptr<SutConnection> m_client_connections[size_t(SutIndexId::max_count)];
	const Handlers		m_handlers;

	std::weak_ptr<AutotestServerInternals> m_server_internals;

};
