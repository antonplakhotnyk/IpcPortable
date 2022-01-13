#pragma once

class MAssIpcThreadSafe
{
public:

	class mutex
	{
	public:
		constexpr void lock()
		{
		}

		constexpr void unlock()
		{
		}
	};

	struct defer_lock_t
	{ // indicates defer lock
		explicit defer_lock_t() = default;
	};


	template<class TMutex>
	class unique_lock
	{
	public:

		constexpr unique_lock(TMutex& mtx)
			: m_mtx(mtx)
		{
			lock();
		}

		constexpr unique_lock(TMutex& mtx, defer_lock_t)
			: m_mtx(mtx)
		{
		}

		~unique_lock()
		{
			if( m_locked )
				m_mtx.unlock();
		}

		constexpr void lock()
		{
			m_mtx.lock();
			m_locked = true;
		}

		constexpr void unlock()
		{
			m_mtx.unlock();
			m_locked = false;
		}

	private:

		TMutex& m_mtx;
		bool m_locked = false;
	};


	class condition_variable
	{
	public:

		void wait(unique_lock<mutex>& lck)
		{
		}

		void notify_all()
		{
		}
	};


	using id = size_t;

	static constexpr id get_id()
	{
		return 1;
	}
};
