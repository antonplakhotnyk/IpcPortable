#pragma once

#include "MAssIpcCall.h"

#include <mutex>
#include <set>
#include <chrono>


class MAssIpcWaiter
{
public:

	enum struct WaitResult
	{
		ready,
		timeout,
		canceled,
	};

	struct ConditionWaitLock
	{
		std::mutex				lock_events;
		std::condition_variable condition_events;
		bool					cancel_all_waits = false;

		std::multiset<const void*> change_filter;
	};


	class Event
	{
	public:

		class Check
		{
		public:
			virtual ~Check() = default;
			virtual bool IsTriggeredLocked() const = 0;
			virtual void ResetLocked() = 0;
			virtual const void* GetFilterId() const = 0;
		};

		Event(const std::shared_ptr<ConditionWaitLock>& cwl)
			:m_cwl(cwl)
		{
		}

		~Event()
		{
			std::unique_lock<std::mutex> lock(m_cwl->lock_events);
			for( const auto& check: *m_checks )
			{
				auto it = m_cwl->change_filter.find(check->GetFilterId());
				if( it != m_cwl->change_filter.end() )
					m_cwl->change_filter.erase(it);
			}

			if( !m_checks->empty() )
				m_cwl->condition_events.notify_all();
		}

		void Reset()
		{
			std::unique_lock<std::mutex> lock(m_cwl->lock_events);
			for( auto& check: *m_checks )
				check->ResetLocked();
		}

		template<class _Rep, class _Period>
		WaitResult Wait(const std::chrono::duration<_Rep, _Period>& rel_time) const
		{
			return WaitDuration(rel_time);
		}

		WaitResult Wait() const
		{
			return WaitDuration<std::nullptr_t>(nullptr);
		}

		bool IsTriggered() const
		{
			std::unique_lock<std::mutex> lock(m_cwl->lock_events);
			return IsTriggeredLocked();
		}

		void BindCheck(const std::shared_ptr<Check>& check, const void* filter_id)
		{
			mass_return_if_equal(bool(check), false);
			std::unique_lock<std::mutex> lock(m_cwl->lock_events);

			m_cwl->change_filter.insert(filter_id);
			m_checks->push_back(check);

			m_cwl->condition_events.notify_all();
		}

		void BindEvent(Event* other_event)
		{
			mass_return_if_equal(bool(other_event), false);
			mass_return_if_not_equal(other_event->m_cwl.get(), m_cwl.get());

			std::unique_lock<std::mutex> lock(m_cwl->lock_events);
	
			if( m_checks!=other_event->m_checks )
			{
				m_checks->insert(m_checks->end(), other_event->m_checks->begin(), other_event->m_checks->end());
				other_event->m_checks = m_checks;
				m_cwl->condition_events.notify_all();
			}
		}

	protected:

		template<class TDuration>
		static bool WaitT(std::unique_lock<std::mutex>& lock, std::condition_variable& condition, const TDuration& rel_time, std::function<bool()> predicate)
		{
			return condition.wait_for(lock, rel_time, predicate);
		}

		template<>
		static bool WaitT<std::nullptr_t>(std::unique_lock<std::mutex>& lock, std::condition_variable& condition, const std::nullptr_t& rel_time, std::function<bool()> predicate)
		{
			condition.wait(lock, predicate);
			return true;
		}

		template<class TDuration>
		WaitResult WaitDuration(const TDuration& rel_time) const
		{
			std::unique_lock<std::mutex> lock(m_cwl->lock_events);
			auto predicate = [this]()
			{
				if( m_cwl->cancel_all_waits )
					return true;
				return IsTriggeredLocked();
			};

			bool wait_ready = WaitT(lock, m_cwl->condition_events, rel_time, predicate);

			return m_cwl->cancel_all_waits ? WaitResult::canceled : (wait_ready ? WaitResult::ready : WaitResult::timeout);
		}

		bool IsTriggeredLocked() const
		{
			for( auto& check: *m_checks )
				if( check->IsTriggeredLocked() )
					return true;
			return false;
		}

	protected:

		const std::shared_ptr<ConditionWaitLock> m_cwl;
		std::shared_ptr<std::vector<std::shared_ptr<Check>>> m_checks = std::make_shared<std::vector<std::shared_ptr<Check>>>();
	};


	class CallEvent: public Event
	{
	private:

		struct Call: public Event::Check
		{
			Call(const std::shared_ptr<const MAssIpcCall::CallInfo>& call_info)
				:m_call_info(call_info)
				, m_wait_start_call_count(call_info->GetCallCount())
			{
			}

			bool IsTriggeredLocked() const override
			{
				if(auto call_info = m_call_info.lock() )
				{
					auto current_count = call_info->GetCallCount();
					if( (current_count - m_wait_start_call_count)>0 )
						return true;
				}
				return false;
			}

			void ResetLocked() override
			{
				if( auto call_info = m_call_info.lock() )
					m_wait_start_call_count = call_info->GetCallCount();
			}


			const void* GetFilterId() const override
			{
				if( auto call_info = m_call_info.lock() )
					return call_info.get();
				return nullptr;
			}

		private:
			const std::weak_ptr<const MAssIpcCall::CallInfo> m_call_info;
			MAssIpcCall::CallInfo::TCounter m_wait_start_call_count = 0;
		};

	public:
		CallEvent(const std::shared_ptr<ConditionWaitLock>& cwl)
			:Event(cwl)
		{
		}

		void BindCall(std::shared_ptr<const MAssIpcCall::CallInfo> event_id)
		{
			BindCheck(std::make_shared<Call>(event_id), event_id.get());
		}

	private:
	};

	void CheckConditionChanged(const void* filter_id)
	{
		std::unique_lock<std::mutex> lock(m_cwl->lock_events);
		if( m_cwl->change_filter.find(filter_id) != m_cwl->change_filter.end() )
			m_cwl->condition_events.notify_all();
	}

	void OnCallCountChanged(const std::shared_ptr<const MAssIpcCall::CallInfo>& call_info)
	{
		CheckConditionChanged(call_info.get());
	}

	std::unique_ptr<MAssIpcWaiter::CallEvent> CreateCallEventBind(const std::initializer_list<std::shared_ptr<const MAssIpcCall::CallInfo> >& event_ids)
	{
		std::unique_ptr<MAssIpcWaiter::CallEvent> new_event{new MAssIpcWaiter::CallEvent(m_cwl)};

		for( auto event_id : event_ids )
			new_event->BindCall(event_id);
		return std::move(new_event);
	}

	template<class TEvent, class... Args>
	std::unique_ptr<TEvent> CreateEvent(Args&&... args)
	{
		static_assert(std::is_base_of<Event, TEvent>::value, "TEvent must be derived from Event");
		return std::unique_ptr<TEvent>{new TEvent(m_cwl, std::forward<Args>(args)...)};
	}

	void CancelAllWaits()
	{
		std::unique_lock<std::mutex> lock(m_cwl->lock_events);
		m_cwl->cancel_all_waits = true;
		m_cwl->condition_events.notify_all();
	}

private:

	const std::shared_ptr<ConditionWaitLock> m_cwl = std::make_shared<ConditionWaitLock>();

};
