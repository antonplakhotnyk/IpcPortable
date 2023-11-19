#pragma once

#include "MAssIpc_Transport.h"


class IpcTransport_MemoryShare: public MAssIpc_TransportShare
{
private:
	struct SyncData
	{
		std::function<void()> on_read;
		std::mutex	lock;
		bool cancel_wait_respond = false;
		std::condition_variable cond;
		std::list<std::unique_ptr<const MAssIpc_Data> > data;
	};

public:

	class MemoryPacket: public MAssIpc_Data
	{
	public:

		MemoryPacket(size_t size)
			:m_data(size)
		{
		}

	private:

		MAssIpc_Data::PacketSize Size() const override
		{
			return MAssIpc_Data::PacketSize(m_data.size());
		}

		uint8_t* Data() override
		{
			return m_data.data();
		}

		const uint8_t* Data() const override
		{
			return m_data.data();
		}

	private:

		std::vector<uint8_t>	m_data;
	};

	IpcTransport_MemoryShare(std::shared_ptr<SyncData> read, std::shared_ptr<SyncData> write)
		:m_read(read)
		, m_write(write)
	{
	}

public:

	IpcTransport_MemoryShare()
		:m_read(new SyncData)
		, m_write(new SyncData)
	{
	}

	void SetHnadler_OnRead(std::function<void()> on_read, bool sync_data_read)
	{
		std::shared_ptr<SyncData>& sync_data = sync_data_read ? m_read : m_write;
		sync_data->on_read = on_read;
	}

	std::shared_ptr<IpcTransport_MemoryShare> CreateComplementar()
	{
		return std::shared_ptr<IpcTransport_MemoryShare>(new IpcTransport_MemoryShare(m_write, m_read));
	}

	bool WaitIncomingData()
	{
		std::unique_lock<std::mutex> lock(m_read->lock);
		while( !m_read->cancel_wait_respond && m_read->data.empty() )
			m_read->cond.wait(lock);
		return !m_read->cancel_wait_respond;
	}

	std::unique_ptr<MAssIpc_Data> Create(MAssIpc_Data::PacketSize size) override
	{
		return std::unique_ptr<MAssIpc_Data>(new MemoryPacket(size));
	}

	bool	Read(bool wait_incoming_packet, std::unique_ptr<const MAssIpc_Data>* packet) override
	{
		std::unique_lock<std::mutex> lock(m_read->lock);
		if( m_read->on_read )
			m_read->on_read();// it not necessary but ipc must survive this
		while( m_read->data.empty() && (!m_read->cancel_wait_respond) )
		{
			if( wait_incoming_packet )
				m_read->cond.wait(lock);
			else
				return true;
		}
		if( m_read->cancel_wait_respond )
			return false;

		*packet = std::move(m_read->data.front());
		m_read->data.pop_front();
		return true;
	}

	void	Write(std::unique_ptr<const MAssIpc_Data> packet) override
	{
		std::lock_guard<std::mutex> lock(m_write->lock);
		SyncData& sync = *m_write;
		sync.data.push_back(std::move(packet));
		sync.cond.notify_all();
	}

	void CancelWaitRespond()
	{
		std::unique_lock<std::mutex> lock(m_read->lock);
		SyncData& sync = *m_read;
		sync.cancel_wait_respond = true;
		sync.cond.notify_all();
	}

private:


	std::shared_ptr<SyncData>	m_write;
	std::shared_ptr<SyncData>	m_read;
};
