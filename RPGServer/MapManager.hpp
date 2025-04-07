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

	RPG::Map* GetMap(const int mapcode_)
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
	void pushJob(const long long elaspedTimeSec_, const std::function<void()>& job_)
	{
		std::chrono::steady_clock::time_point ReqTime = std::chrono::steady_clock::now() + std::chrono::seconds(elaspedTimeSec_);

		ToDoQueue.push(ReqTime, job_);

		return;
	}

	void ReleaseItemObject(ItemObject* pItem_)
	{
		if (pItem_ == nullptr)
		{
			return;
		}

		pItem_->Clear();

		m_ItemObjectPool->Deallocate(pItem_);
		return;
	}

	std::function<void(std::map<unsigned short, User*>&, RESULTCODE, std::string&, int)> SendInfoToUsersFunc;
	std::function<void(const unsigned short, RESULTCODE, std::string&)> SendInfoFunc;

private:
	RPG::Map* CreateMap(int mapcode_)
	{
		RPG::Map* pMap = new RPG::Map(mapcode_);
		pMap->SendInfoToUsersFunc = this->SendInfoToUsersFunc;
		pMap->SendInfoFunc = this->SendInfoFunc;

		auto allocateFunc = [this]() ->ItemObject* {return m_ItemObjectPool->Allocate(); };
		auto DeallocateFunc = [this](ItemObject* pItem_) {ReleaseItemObject(pItem_); };

		pMap->AllocateItemObject = allocateFunc;
		pMap->DeallocateItemObject = DeallocateFunc;

		auto reserveJobFunc = [this](const long long elaspedTimeSec, const std::function<void()>& job) {pushJob(elaspedTimeSec, job); };

		pMap->ReserveJob = reserveJobFunc;

		m_mapList.emplace(mapcode_, pMap);

		std::cout << "MapManager::CreateMap : Created Map : code" << mapcode_ << "\n";

		return pMap;
	}

	Time_Based_PriorityQueue ToDoQueue;
	std::unordered_map<int, RPG::Map*> m_mapList;
	std::unique_ptr<MemoryPool<ItemObject>> m_ItemObjectPool;
};