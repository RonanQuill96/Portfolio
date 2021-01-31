#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

template<class DataType>
class ConcurrentQueue
{
public:
	ConcurrentQueue() = default;

	~ConcurrentQueue()
	{
		Invalidate();
	}

	bool TryPop(DataType& out)
	{
		std::lock_guard<std::mutex> lockguard(queueMutex);
		if (queue.empty() || !valid)
		{
			return false;
		}

		out = std::move(queue.front());
		queue.pop();

		return true;
	}

	bool WaitPop(DataType& out)
	{
		std::unique_lock<std::mutex> lock(queueMutex);
		condition.wait(lock, [this]() { return !queue.empty() || !valid || close; }); //wait until the workQueue is not empty

		if (queue.empty() || !valid)
		{
			return false;
		}

		out = std::move(queue.front());
		queue.pop();

		return true;
	}

	void Pop()
	{
		std::lock_guard<std::mutex> lockguard(queueMutex);
		queue.pop();
	}

	void Push(DataType event)
	{
		std::lock_guard<std::mutex> lockguard(queueMutex);
		if (!close)
		{
			queue.push(event);
			condition.notify_one();
		}
	}

	void MovePush(DataType event)
	{
		std::lock_guard<std::mutex> lockguard(queueMutex);
		if (!close)
		{
			queue.push(std::move(event));
			condition.notify_one();
		}
	}

	size_t Size()
	{
		std::lock_guard<std::mutex> lockguard(queueMutex);
		return queue.size();
	}

	bool Empty()
	{
		std::lock_guard<std::mutex> lockguard(queueMutex);
		return queue.empty();
	}

	void Clear()
	{
		std::lock_guard<std::mutex> lockguard(queueMutex);
		queue = {};
		condition.notify_all();
	}

	void Invalidate()
	{
		std::lock_guard<std::mutex> lockguard(queueMutex);
		valid = false;
		condition.notify_all();
	}

	void CloseQueue()
	{
		std::lock_guard<std::mutex> lockguard(queueMutex);
		close = true;
		condition.notify_all();
	}

private:
	std::mutex queueMutex;
	std::condition_variable condition;
	std::queue<DataType> queue;
	bool valid = true;
	bool close = false;
};
