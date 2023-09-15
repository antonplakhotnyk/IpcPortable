#pragma once

#include "MAssIpc_Transport.h"
#include <mutex>
#include <condition_variable>


class IpcTransport_MemoryCopy: public MAssIpc_TransportCopy
{
private:
	struct SyncData
	{
		std::function<void()> on_read;
		std::mutex	lock;
		bool incoming_data = false;
		bool cancel_wait_respond = false;
		std::condition_variable cond;
		std::vector<uint8_t> data;
	};

	IpcTransport_MemoryCopy(std::shared_ptr<SyncData> read, std::shared_ptr<SyncData> write)
		:m_read(read)
		, m_write(write)
	{
	}

public:

	IpcTransport_MemoryCopy()
		:m_read(new SyncData)
		, m_write(new SyncData)
	{
	}

	void SetHnadler_OnRead(std::function<void()> on_read, bool sync_data_read)
	{
		std::shared_ptr<SyncData>& sync_data = sync_data_read ? m_read : m_write;
		sync_data->on_read = on_read;
	}

	std::shared_ptr<IpcTransport_MemoryCopy> CreateComplementar()
	{
		return std::shared_ptr<IpcTransport_MemoryCopy>(new IpcTransport_MemoryCopy(m_write, m_read));
	}

	void WaitIncomingData()
	{
		std::unique_lock<std::mutex> lock(m_read->lock);
		while( !m_read->incoming_data )
			m_read->cond.wait(lock);
	}

	bool	WaitRespond(size_t expected_size) override
	{
		std::unique_lock<std::mutex> lock(m_read->lock);
		while( (m_read->data.size()<expected_size) && (!m_read->cancel_wait_respond) )
			m_read->cond.wait(lock);
		return !m_read->cancel_wait_respond;
	}

	size_t	ReadBytesAvailable() override
	{
		std::lock_guard<std::mutex> lock(m_read->lock);
		return m_read->data.size();
	}

	void	Read(uint8_t* data, size_t size) override
	{
		std::lock_guard<std::mutex> lock(m_read->lock);
		SyncData& sync = *m_read;
		if( sync.on_read )
			sync.on_read();// it not necessary but ipc must survive this
		if( sync.data.size()<size )
			return;// unexpected
		std::copy(sync.data.begin(), sync.data.begin()+size, data);
		sync.data.erase(sync.data.begin(), sync.data.begin()+size);
		sync.incoming_data = false;
	}

	void	Write(const uint8_t* data, size_t size) override
	{
		std::lock_guard<std::mutex> lock(m_write->lock);
		SyncData& sync = *m_write;
		sync.data.insert(sync.data.end(), data, data+size);
		sync.incoming_data = true;
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
