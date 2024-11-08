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
		WorkThread.join();
	}

	void push(time_t reserved_time_, Func job_)
	{
		std::lock_guard<std::mutex> guard(m_mutex);
		q.emplace(reserved_time_, job_);
	}

private:
	struct cmp
	{
		bool operator()(const std::pair<time_t, Func>& p1, const std::pair<time_t, Func>& p2)
		{
			return p1.first > p2.first;
		}
	};

	void Do()
	{
		// top�� ��ȸ�ϴ� ������� �ϳ��� �� ���̱� ������ lock�� �Ŵ� ��ġ�� �ļ���.
		while (m_isRun)
		{
			time_t currentTime = time(NULL);
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

	std::priority_queue<std::pair<time_t, Func>, std::vector<std::pair<time_t, Func>>, cmp> q;
};