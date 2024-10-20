#pragma once

#include "GameMap.hpp"
#include <map>

class MapManager
{
public:
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
	}

	RPG::Map* GetMap(int mapcode_)
	{
		auto itr = m_mapList.find(mapcode_);

		if (itr != m_mapList.end())
		{
			return itr->second;
		}

		return CreateMap(mapcode_);
	}

	std::function<void()> SendPacketFunc;
	std::function<PacketData* ()> AllocatePacket;

private:
	RPG::Map* CreateMap(int mapcode_)
	{
		RPG::Map* pMap = new RPG::Map(mapcode_);
		m_mapList.emplace(mapcode_, pMap);

		return pMap;
	}

	std::map<int, RPG::Map*> m_mapList;
};