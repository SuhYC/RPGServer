#pragma once

#include "Vector.hpp"
#include <map>
#include <atomic>

class Monster
{
public:
	void Init(const unsigned int monsterCode_, const unsigned int MaxHealthPoint, const Vector2& position_)
	{
		m_MonsterCode = monsterCode_;
		m_MaxHealthPoint = MaxHealthPoint;
		m_SpawnPosition = position_;

		Spawn();
	}

	bool Spawn()
	{
		if (m_bIsAlive)
		{
			return false;
		}

		Clear();
		m_HealthPoint = m_MaxHealthPoint;
		m_position = m_SpawnPosition;
		m_bIsAlive = true;

		return true;
	}

	// ret : if Monster died -> who got the reward
	// ret 0 : Monster still alive
	int GetDamaged(const int usercode_, const unsigned int damage_)
	{
		std::lock_guard<std::mutex> guard(m_mutex);

		if (m_bIsAlive == false)
		{
			return 0;
		}

		auto itr = m_CumulativeDamage.find(usercode_);
		if (itr == m_CumulativeDamage.end())
		{
			m_CumulativeDamage.emplace(usercode_, 0);
			itr = m_CumulativeDamage.find(usercode_);
		}

		if (m_HealthPoint < damage_)
		{
			m_bIsAlive = false;
			itr->second += m_HealthPoint;

			unsigned int maxDamage = itr->second;
			int maxDamageUser = usercode_;

			for (auto& i : m_CumulativeDamage)
			{
				if (maxDamage < i.second)
				{
					maxDamageUser = i.first;
					maxDamage = i.second;
				}
			}

			return maxDamageUser;
		}

		itr->second += damage_;
		m_HealthPoint -= damage_;
		return 0;
	}

	unsigned int GetMonsterCode() const { return m_MonsterCode; }

	bool IsAlive() const { return m_bIsAlive; }

private:
	// 정보를 아예 초기화하는 함수다. 보상처리가 끝날때까지 사용하지 말것.
	void Clear()
	{
		m_CumulativeDamage.clear();
	}

	std::mutex m_mutex;

	unsigned int m_MonsterCode;
	unsigned int m_MaxHealthPoint;
	Vector2 m_SpawnPosition;

	// --가변량
	std::atomic<unsigned int> m_HealthPoint;
	std::atomic<Vector2> m_position;
	std::atomic<bool> m_bIsAlive;

	std::map<int, unsigned int> m_CumulativeDamage;
};