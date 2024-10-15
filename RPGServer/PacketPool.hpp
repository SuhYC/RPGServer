#pragma once
#include <concurrent_queue.h>
#include "PacketData.hpp"

namespace NetworkPacket
{
	const int OBJECT_COUNT = 100;

	class PacketPool
	{
	public:
		PacketPool()
		{
			for (int i = 0; i < OBJECT_COUNT; i++)
			{
				q.push(new PacketData());
			}
		}

		~PacketPool()
		{
			PacketData* tmp;
			while (q.try_pop(tmp))
			{
				delete tmp;
			}
		}

		void Deallocate(PacketData* pPacket_)
		{
			q.push(pPacket_);
		}

		PacketData* Allocate()
		{
			PacketData* ret = nullptr;

			if (q.try_pop(ret))
			{
				return ret;
			}

			std::cerr << "메모리 풀 부족\n";
			ret = new PacketData();

			return ret;
		}

	private:
		Concurrency::concurrent_queue<PacketData*> q;
	};

}

