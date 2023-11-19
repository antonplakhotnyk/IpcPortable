#pragma once

#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>


class TaskRunnerThread
{
	struct TaskBase
	{
		virtual ~TaskBase()=default;
		virtual void Invoke()=0;
	};

	template<class TRet>
	struct Task: TaskBase
	{
	private:

		struct SetPromise_TRet
		{
			static void Value(const std::function<TRet()>& task, std::promise<TRet>& promise)
			{
				promise.set_value(task());
			}
		};

		// Nested class to handle void type
		struct SetPromise_Void
		{
			static void Value(const std::function<TRet()>& task, std::promise<TRet>& promise)
			{
				task();
				promise.set_value();
			}
		};

		using SetPromise = typename std::conditional<std::is_void<TRet>::value, SetPromise_Void, SetPromise_TRet>::type;

	public:
		const std::function<TRet()> task;
		std::promise<TRet> promise;

		Task(std::function<TRet()>&& task)
			:task(std::move(task))
		{
		}

		void Invoke() override
		{
			SetPromise::Value(task, promise);
		}
	};

public:
	TaskRunnerThread()
	{
		m_thread = std::thread([this]
		{
			Run();
		});
	}

	~TaskRunnerThread()
	{
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_exit = true;
			m_cond_var.notify_one();
		}
		m_thread.join();
	}

	template<class TRet>
	std::future<TRet> PostTask(std::function<TRet()> task)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		std::unique_ptr<Task<TRet>> p_task{new Task<TRet>(std::move(task))};

		auto future = p_task->promise.get_future();
		m_task_queue.push(std::move(p_task));

		m_cond_var.notify_one();
		return future;
	}

	std::future<void> PostTask(std::function<void()> task)
	{
		return PostTask<void>(std::move(task));
	}

	template<class TRet>
	std::tuple<std::future_status, TRet>  WaitTask(std::chrono::milliseconds ms, std::function<TRet()> task)
	{
		std::future<TRet> task_res = PostTask<TRet>(std::move(task));
		auto ws = task_res.wait_for(ms);
		return {ws, task_res.get()};
	}

	std::future_status WaitTask(std::chrono::milliseconds ms, std::function<void()> task)
	{
		std::future<void> task_res = PostTask<void>(std::move(task));
		return task_res.wait_for(ms);
	}

	template<class TRet>
	TRet  WaitTask(std::function<TRet()> task)
	{
		std::future<TRet> task_res = PostTask<TRet>(std::move(task));
		task_res.wait();
		return task_res.get();
	}

	void WaitTask(std::function<void()> task)
	{
		std::future<void> task_res = PostTask<void>(std::move(task));
		task_res.wait();
	}

private:

	void Run()
	{
		while( !m_exit || !m_task_queue.empty() )
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			m_cond_var.wait(lock, [this]
			{
				return !m_task_queue.empty() || m_exit;
			});

			if( !m_task_queue.empty() )
			{
				auto task = std::move(m_task_queue.front());
				m_task_queue.pop();
				lock.unlock();

				task->Invoke();
			}
		}
	}

	std::thread m_thread;
	std::queue<std::unique_ptr<TaskBase>> m_task_queue;
	std::mutex m_mutex;
	std::condition_variable m_cond_var;
	bool m_exit = false;
};

