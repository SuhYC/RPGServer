#pragma once

#include "Define.hpp"
#include <concurrent_queue.h>
#include <hiredis/hiredis.h>

namespace Redis
{
	const char* REDIS_IP = "127.0.0.1";
	const int REDIS_PORT = 6379;
	const int OBJECT_COUNT = 3;

	class ConnectionPool
	{
	public:
		ConnectionPool()
		{
			redisContext* rc = nullptr;
			for (int i = 0; i < OBJECT_COUNT; i++)
			{
				rc = redisConnect(REDIS_IP, REDIS_PORT);
				if (rc != nullptr)
				{
					q.push(rc);
				}
				else
				{
					std::cerr << "RedisConnectionPool::redisConnect : Redis연결 실패...\n";
				}
			}
			std::cout << "Redis작업 완료...\n";
		}

		~ConnectionPool()
		{
			int cnt = 0;
			redisContext* tmp;
			while (q.try_pop(tmp))
			{
				cnt++;
				redisFree(tmp);
			}

			std::cout << "redisContext:" << cnt << "\n";
		}

		void Deallocate(redisContext* pRedisContext_)
		{
			q.push(pRedisContext_);
		}

		redisContext* Allocate()
		{
			redisContext* ret = nullptr;

			if (q.try_pop(ret))
			{
				return ret;
			}

			std::cerr << "메모리 풀 부족\n";
			ret = redisConnect(REDIS_IP, REDIS_PORT);

			return ret;
		}

	private:
		Concurrency::concurrent_queue<redisContext*> q;
	};

}
