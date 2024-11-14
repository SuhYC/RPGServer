#pragma once

#include "GameMap.hpp"
#include <map>
#include "Time_Based_PriorityQueue.hpp"
#include "MemoryPool.hpp"
#include <unordered_map>

const int ITEM_OBJECT_POOL_INSTANCE_COUNT = 100;

class MapManager
{
public:
	MapManager()
	{
		m_ItemObjectPool = std::make_unique<MemoryPool<ItemObject>>(ITEM_OBJECT_POOL_INSTANCE_COUNT);
	}

	~MapManager()
	{
		for (auto& i : m_mapList)
		{
			if ((i.second) != nullptr)
			{
				delete i.second;
				i.second = nullptr;
			}
		}

		m_ItemObjectPool.reset();
	}

	RPG::Map* GetMap(int mapcode_)
	{
		if (mapcode_ < 1)
		{
			return nullptr;
		}

		auto itr = m_mapList.find(mapcode_);

		if (itr != m_mapList.end())
		{
			return itr->second;
		}

		return CreateMap(mapcode_);
	}

	// 여기까지는 function을 참조로 보내도 된다.
	// 큐에 넣은 이후 람다는 소멸하기 때문에 큐 내부에서 받을때는 복사로 받는다.
	void pushJob(const time_t elaspedTime_, const std::function<void()>& job_)
	{
		time_t currentTime = time(NULL);

		ToDoQueue.push(currentTime + elaspedTime_, job_);

		return;
	}

	void ReleaseItemObject(ItemObject* pItem_)
	{
		m_ItemObjectPool->Deallocate(pItem_);
		return;
	}


	std::function<PacketData*()> AllocatePacket;
	std::function<void(PacketData*)> DeallocatePacket;
	std::function<bool(PacketData*)> SendMsgFunc;

private:
	RPG::Map* CreateMap(int mapcode_)
	{
		RPG::Map* pMap = new RPG::Map(mapcode_);
		pMap->AllocatePacket = AllocatePacket;
		pMap->DeallocatePacket = DeallocatePacket;
		pMap->SendMsgFunc = SendMsgFunc;

		auto reserveJobFunc = [this](const time_t elaspedTime, const std::function<void()>& job) {pushJob(elaspedTime, job); };

		pMap->ReserveJob = reserveJobFunc;

		m_mapList.emplace(mapcode_, pMap);

		return pMap;
	}

	Time_Based_PriorityQueue ToDoQueue;
	std::unordered_map<int, RPG::Map*> m_mapList;
	std::unique_ptr<MemoryPool<ItemObject>> m_ItemObjectPool;
};