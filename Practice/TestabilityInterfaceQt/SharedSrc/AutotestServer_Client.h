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

	enum struct SutIndexId:size_t
	{
		on_connected_sut = 0,
		max_count = 16,
	};

	AutotestServer_Client(const std::weak_ptr<AutotestClient_Internals>& server_internals)
		: m_client_internals(server_internals)
	{
	}

	const MAssIpcCall& IpcCall(SutIndexId sut_index_id = {}) const
	{
		std::unique_lock<std::recursive_mutex> lock(m_connections_mutex);

		if( sut_index_id >=SutIndexId::max_count )
			return m_disconnected_stub;
		if( !bool(m_client_connections[size_t(sut_index_id)]) )
			return m_disconnected_stub;

		return m_client_connections[size_t(sut_index_id)]->ipc_net.ipc_call;
	};

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

	std::map<std::weak_ptr<IpcQt_TransporTcp>, std::shared_ptr<SutConnection>, std::owner_less<std::weak_ptr<IpcQt_TransporTcp>> >	m_connections;
	std::shared_ptr<SutConnection>			m_client_connections[size_t(SutIndexId::max_count)];
	mutable std::recursive_mutex			m_connections_mutex;

	const std::shared_ptr<AutotestClient_Internals> m_client_internals;

};
