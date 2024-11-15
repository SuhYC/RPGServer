#pragma once

#include <queue>
#include <functional>
#include <thread>
#include <mutex>

typedef std::function<void()> Func;
class Time_Based_PriorityQueue
{
public:
	Time_Based_PriorityQueue() : m_isRun(true)
	{
		WorkThread = std::thread([this]() {Do(); });
	}

	~Time_Based_PriorityQueue()
	{
		m_isRun = false;

		if (WorkThread.joinable())
		{
			WorkThread.join();
		}
	}

	void push(std::chrono::steady_clock::time_point& reserved_time_, Func job_)
	{
		std::lock_guard<std::mutex> guard(m_mutex);
		q.emplace(reserved_time_, job_);
	}

private:
	struct cmp
	{
		bool operator()(const std::pair<std::chrono::steady_clock::time_point, Func>& p1, const std::pair<std::chrono::steady_clock::time_point, Func>& p2)
		{
			return p1.first > p2.first;
		}
	};

	void Do()
	{
		// top을 조회하는 스레드는 하나만 둘 것이기 때문에 lock을 거는 위치는 후순위.
		while (m_isRun)
		{
			std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
			if (q.empty() || q.top().first > currentTime)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				continue;
			}

			q.top().second();
			std::lock_guard<std::mutex> guard(m_mutex);
			q.pop();
		}
	}

	std::mutex m_mutex;
	std::thread WorkThread;

	bool m_isRun;

	std::priority_queue<std::pair<std::chrono::steady_clock::time_point, Func>, std::vector<std::pair<std::chrono::steady_clock::time_point, Func>>, cmp> q;
};