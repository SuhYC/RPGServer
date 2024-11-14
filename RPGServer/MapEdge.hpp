#pragma once

#include <unordered_map>

class MapEdge
{
public:
	// 서버 가동 전 해당 함수로 MapEdge에 대한 정보를 기록하여야한다.
	void Insert(const int fromMapCode_, const int toMapCode_) noexcept
	{
		m_mapEdge.emplace(fromMapCode_, toMapCode_);

		return;
	}

	// from맵에서 to맵으로 갈 수 있는지 확인하는 함수
	// 한 맵에서 갈 수 있는 경로가 많지 않을 것이기 때문에 multimap을 통해 조회한다.
	// fromMapcode_로 조회하는 과정에서 O(1), 조회결과에서 toMapCode_를 확인하는데 O(n)
	bool Find(const int fromMapCode_, const int toMapCode_)
	{
		auto range = m_mapEdge.equal_range(fromMapCode_);

		for (auto& itr = range.first; itr != range.second; itr++)
		{
			if (itr->second == toMapCode_)
			{
				return true;
			}
		}

		return false;
	}

private:
	std::unordered_multimap<int, int> m_mapEdge; // param1맵에서 param2맵으로 이동가능함을 나타냄. -> DB작업 필요
};