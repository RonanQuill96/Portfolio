#pragma once

#include "ConcurrentQueue.h"

#include <future>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool
{
public:
	ThreadPool(const size_t desiredThreadCount = std::thread::hardware_concurrency())
	{
		threadCount = desiredThreadCount;
		if (threadCount == 0)
		{
			threadCount = 1;
		}

		threads.reserve(threadCount);
		for (size_t threadIndex = 0; threadIndex < threadCount; ++threadIndex)
		{
			threads.emplace_back([this]()
				{
					std::function<void()> Task;

					while (this->workQueue.WaitPop(Task))
					{
						Task(); //execute task;
					}
				});
		}
	}

	~ThreadPool() 
	{
		Stop();
	}

	void Stop(bool wait = false)
	{
		if (!wait)
		{
			workQueue.Invalidate();
		}
		else
		{
			workQueue.CloseQueue();
		}

		for (std::thread& thread : threads)
		{
			thread.join();
		}

		threads.clear();
	}

	size_t GetThreadCount() const
	{
		return threadCount;
	}

	template<typename TaskFunction, class... Arguements>
	auto AddTask(TaskFunction&& function, Arguements&&... arguements) -> std::future<decltype(function(arguements...))>
	{
		using return_type = decltype(function(arguements...));

		auto task = std::make_shared< std::packaged_task<return_type()> >
			(
				std::bind(std::forward<TaskFunction>(function), std::forward<Arguements>(arguements)...)
				);


		workQueue.Push([task]() { (*task)(); });

		return task->get_future();
	}

private:
	size_t threadCount = 0;

	std::vector<std::thread> threads;
	ConcurrentQueue<std::function<void()>> workQueue;
};


