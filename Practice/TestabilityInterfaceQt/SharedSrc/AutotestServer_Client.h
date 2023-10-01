#pragma once

#include "IpcQt_Net.h"
#include "MAssIpcWaiter.h"

//-------------------------------------------------------

class AutotestClient_Internals
{
public:

	AutotestClient_Internals()
	{
		m_disconnect_event->BindConnect(MAssIpcWaiter::ConnectionEvent::State::on_disconnected, m_connection_signaling);
	}

public:

	const std::shared_ptr<MAssIpcWaiter>		m_waiter = std::make_shared<MAssIpcWaiter>();
	const std::shared_ptr<MAssIpcWaiter::ConnectionEvent::Signaling> m_connection_signaling = m_waiter->CreateSignaling<MAssIpcWaiter::ConnectionEvent>();
	const std::unique_ptr<MAssIpcWaiter::ConnectionEvent> m_disconnect_event{m_waiter->CreateEvent<MAssIpcWaiter::ConnectionEvent>()};
};

//-------------------------------------------------------

class AutotestServer_Client
{

public:

	struct SutConnection
	{
		SutConnection(MAssIpcCall& ipc_connection, std::shared_ptr<IpcQt_TransporTcp> transport, std::weak_ptr<AutotestClient_Internals> internals)
			: ipc_net(ipc_connection, transport)
			, m_internals{internals}
		{
			this->ipc_net.ipc_call.SetHandler_ErrorOccured(&SutConnection::IpcError, this);

			if(auto internals = m_internals.lock())
			{
				std::unique_ptr<MAssIpcWaiter::CallEvent::Signaling> call_signaling = internals->m_waiter->CreateSignaling<MAssIpcWaiter::CallEvent>();

				this->ipc_net.ipc_call.SetHandler_CallCountChanged([call_signaling = std::move(call_signaling)](const std::shared_ptr<const MAssIpcCall::CallInfo>& call_info)
				{
					call_signaling->OnCallCountChanged(call_info);
				}, this);
			}
		}

		~SutConnection()
		{
			this->ipc_net.ipc_call.ClearHandlersWithTag(this);
		}

		IpcQt_Net		ipc_net;

	private:
		static	void IpcError(MAssIpcCall::ErrorType et, const std::string& message);

	private:
		const std::weak_ptr<AutotestClient_Internals> m_internals;
	};

public:

	AutotestServer_Client(const std::weak_ptr<AutotestClient_Internals>& server_internals)
		: m_client_internals(server_internals)
	{
	}

	enum struct SutIndexId:size_t
	{
		on_connected_sut = 0,
		max_count = 16,
	};

	const MAssIpcCall& IpcCall(SutIndexId sut_index_id = {}) const
	{
		std::unique_lock<std::recursive_mutex> lock(m_connections_mutex);

		if( sut_index_id >=SutIndexId::max_count )
			return m_disconnected_stub;
		if( !bool(m_client_connections[size_t(sut_index_id)]) )
			return m_disconnected_stub;

		return m_client_connections[size_t(sut_index_id)]->ipc_net.ipc_call;
	};


	enum struct ConnectionUsage
	{
		newly_connected,
		not_used,
		use_ipc_call,
	};

	using ConnectionId = std::weak_ptr<IpcQt_TransporTcp>;

	bool Connections_IsValidId(const ConnectionId& connection_id) const
	{
		std::unique_lock<std::recursive_mutex> lock(m_connections_mutex);
		auto it = m_connections.find(connection_id);
		return (it != m_connections.end());
	}

	bool Connections_IsEqual(const ConnectionId& a, const ConnectionId& b) const
	{
		std::unique_lock<std::recursive_mutex> lock(m_connections_mutex);
		auto comp = m_connections.key_comp();
		return !comp(a, b) && !comp(b, a);
	}

	void Connections_SetIpcUse(ConnectionId connection_id, SutIndexId sut_index_id)
	{
		std::unique_lock<std::recursive_mutex> lock(m_connections_mutex);

		if(sut_index_id==SutIndexId::on_connected_sut)
			sut_index_id = SutIndexId::max_count;

		auto it = m_connections.find(connection_id);
		if( it != m_connections.end() )
			if( SetConnectionLocked(sut_index_id, it->second.connection) )
				it->second.usage = ConnectionUsage::use_ipc_call;
	}

	void Connections_SetNotUsed(ConnectionId connection_id)
	{
		std::unique_lock<std::recursive_mutex> lock(m_connections_mutex);

		auto it = m_connections.find(connection_id);
		if( it != m_connections.end() )
			it->second.usage = ConnectionUsage::not_used;
	}

	void Connections_ClearIpcUse(SutIndexId sut_index_id)
	{
		std::unique_lock<std::recursive_mutex> lock(m_connections_mutex);

		if( (sut_index_id >=SutIndexId::max_count) || (sut_index_id ==SutIndexId::on_connected_sut) )
			return;
		std::shared_ptr<SutConnection> connection = m_client_connections[size_t(sut_index_id)];
		if( !bool(connection) )
			return;

		SetConnectionLocked(sut_index_id, {});

		static_assert(size_t(SutIndexId::on_connected_sut)==0, "expected 0");
		auto it_client_connection = std::find_if(std::begin(m_client_connections)+size_t(SutIndexId::on_connected_sut)+1, std::end(m_client_connections),
								  [&](const std::shared_ptr<SutConnection>& sut_connection)
		{
			return sut_connection == connection;
		});

		if( it_client_connection == std::end(m_client_connections) )
		{
			auto it_connection = std::find_if(m_connections.begin(), m_connections.end(), [&](const decltype(m_connections)::value_type& entry)
			{
				return entry.second.connection == connection;
			});

			if( it_connection!=m_connections.end() )
				it_connection->second.usage = ConnectionUsage::not_used;
		}
	}

	const MAssIpcCall& Connections_GetIpc(ConnectionId connection_id) const
	{
		std::unique_lock<std::recursive_mutex> lock(m_connections_mutex);

		auto it = m_connections.find(connection_id);
		if( it != m_connections.end() )
			return it->second.connection->ipc_net.ipc_call;
		else
			return m_disconnected_stub;
	}

	std::vector<ConnectionId> Connections_GetWithUsage(ConnectionUsage usage) const
	{
		std::unique_lock<std::recursive_mutex> lock(m_connections_mutex);

		std::vector<ConnectionId> filtered_connections;
		for( const auto& entry : m_connections )
			if( entry.second.usage == usage )
				filtered_connections.push_back(entry.first);

		return filtered_connections;
	}

protected:

	void AddNewConnection(const std::weak_ptr<IpcQt_TransporTcp>& transport, std::shared_ptr<SutConnection> connection)
	{
		std::unique_lock<std::recursive_mutex> lock(m_connections_mutex);
		m_connections.insert(std::make_pair(transport, ConnectionState{connection}));
		SetConnectionLocked(SutIndexId(0), connection);
	}

	void RemoveConnection(const std::weak_ptr<IpcQt_TransporTcp>& transport)
	{
		std::unique_lock<std::recursive_mutex> lock(m_connections_mutex);
		
		auto it = m_connections.find(transport);

		if( it != m_connections.end() )
		{
			const auto& sut_connection = it->second.connection;
			for( size_t i = 0; i < std::extent<decltype(m_client_connections)>::value; ++i )
				if( m_client_connections[i] == sut_connection )
					m_client_connections[i].reset();

			m_connections.erase(it);
		}
	}

	bool SetConnectionLocked(SutIndexId sut_id, std::shared_ptr<SutConnection> connection);

public:

	class Event: public MAssIpcWaiter::Event
	{
	public:
		Event(const std::shared_ptr<MAssIpcWaiter::ConditionWaitLock>& cwl, std::shared_ptr<AutotestClient_Internals> internals, bool skip_canceling_by_disconnect)
			: MAssIpcWaiter::Event(cwl)
			, m_internals{internals}
		{
			mass_return_if_equal(bool(m_internals), false);

			if( !skip_canceling_by_disconnect )
			{
				const std::unique_ptr<MAssIpcWaiter::ConnectionEvent> disconnect_event{m_internals->m_waiter->CreateEvent<MAssIpcWaiter::ConnectionEvent>()};
				disconnect_event->BindConnect(MAssIpcWaiter::ConnectionEvent::State::on_disconnected, m_internals->m_connection_signaling, InitialCount::current, true);
				this->BindEvent(disconnect_event.get());
			}
		}

		enum struct LocalId
		{
			ConnectedSut,
			DisconnectedSut,
		};

		using InitialCount = MAssIpcWaiter::CallEvent::InitialCount;

		void BindLocal(LocalId event_id)
		{
			switch( event_id )
			{
				case LocalId::ConnectedSut:
				case LocalId::DisconnectedSut:
				{
					std::unique_ptr<MAssIpcWaiter::ConnectionEvent> connect_event{m_internals->m_waiter->CreateEvent<MAssIpcWaiter::ConnectionEvent>()};
					MAssIpcWaiter::ConnectionEvent::State signal_state = (event_id == LocalId::ConnectedSut) ? MAssIpcWaiter::ConnectionEvent::State::on_connected : MAssIpcWaiter::ConnectionEvent::State::on_disconnected;
					connect_event->BindConnect(signal_state, m_internals->m_connection_signaling);
					this->BindEvent(connect_event.get());
				}
				break;
			default:
				mass_assert();
			    break;
			}
		}

		void BindSut(std::shared_ptr<const MAssIpcCall::CallInfo> event_id, InitialCount initial_count = InitialCount::current)
		{
			std::unique_ptr<MAssIpcWaiter::CallEvent> call_event{m_internals->m_waiter->CreateEvent<MAssIpcWaiter::CallEvent>()};
			call_event->BindCall(event_id, initial_count);
			this->BindEvent(call_event.get());
		}


	private:

		const std::shared_ptr<AutotestClient_Internals> m_internals;
	};

	std::unique_ptr<Event> CreateEvent(bool skip_canceling_by_disconnect = false)
	{
		return m_client_internals->m_waiter->CreateEvent<Event>(m_client_internals, skip_canceling_by_disconnect);
	}

	std::unique_ptr<Event> CreateEventSut(std::shared_ptr<const MAssIpcCall::CallInfo> event_id, Event::InitialCount initial_count = Event::InitialCount::current, bool skip_canceling_by_disconnect = false)
	{
		std::unique_ptr<Event> event_new = CreateEvent(skip_canceling_by_disconnect);
		event_new->BindSut(event_id, initial_count);
		return event_new;
	}

private:
	MAssIpcCall		m_disconnected_stub{std::weak_ptr<MAssIpc_Transthread>{}};
protected:


	struct ConnectionState
	{
		std::shared_ptr<SutConnection> connection;
		ConnectionUsage usage = ConnectionUsage::newly_connected;
	};

	std::map<std::weak_ptr<IpcQt_TransporTcp>, ConnectionState, std::owner_less<std::weak_ptr<IpcQt_TransporTcp>> >	m_connections;
	std::shared_ptr<SutConnection>			m_client_connections[size_t(SutIndexId::max_count)];
	mutable std::recursive_mutex			m_connections_mutex;

	const std::shared_ptr<AutotestClient_Internals> m_client_internals;

};
