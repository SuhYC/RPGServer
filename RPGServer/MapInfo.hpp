#pragma once

#include <unordered_map>

class MapInfo
{
public:
	// 서버 가동 전 해당 함수로 정보를 기록하여야한다.
	void InsertMapNPCInfo(const int mapCode_, const int NPCCode_) noexcept
	{
		m_NPCInfo.emplace(mapCode_, NPCCode_);

		return;
	}

	// 서버 가동 전 해당 함수로 정보를 기록하여야한다.
	void InsertMapMonsterInfo(const int mapCode_, const int monsterCode_) noexcept
	{
		m_MonsterInfo.emplace(mapCode_, monsterCode_);

		return;
	}

	bool FindMapNPCInfo(const int mapCode_, const int NPCCode_)
	{
		auto range = m_NPCInfo.equal_range(mapCode_);

		for (auto& itr = range.first; itr != range.second; itr++)
		{
			if (itr->second == NPCCode_)
			{
				return true;
			}
		}

		return false;
	}

	bool FindMapMonsterInfo(const int mapCode_, const int monsterCode_)
	{
		auto range = m_MonsterInfo.equal_range(mapCode_);

		for (auto& itr = range.first; itr != range.second; itr++)
		{
			if (itr->second == monsterCode_)
			{
				return true;
			}
		}

		return false;
	}

	std::pair<std::unordered_multimap<int, int>::iterator, std::unordered_multimap<int, int>::iterator>&& Get_Map_MonsterInfo(const int mapCode_)
	{
		return m_MonsterInfo.equal_range(mapCode_);
	}

private:
	std::unordered_multimap<int, int> m_NPCInfo; // <mapcode, npccode>
	std::unordered_multimap<int, int> m_MonsterInfo; // <mapcode, monstercode>
};