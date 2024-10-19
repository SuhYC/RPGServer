#pragma once

#include "Vector.hpp"
#include <map>

class Monster
{
public:
	void Init(const unsigned int monsterCode_, const unsigned int MaxHealthPoint, Vector2& position_, Vector2& vel_)
	{
		m_MonsterCode = monsterCode_;
		m_HealthPoint = MaxHealthPoint;
		m_position = position_;
		m_velocity = vel_;
		m_bIsAlive = true;
	}

	// ret : if Monster died -> who got the reward
	// ret 0 : Monster still alive
	int GetDamaged(const int usercode_, const unsigned int damage_)
	{
		std::lock_guard<std::mutex> guard(m);

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

			int maxDamage = itr->second;
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

	// 정보를 아예 초기화하는 함수다. 보상처리가 끝날때까지 사용하지 말것.
	void Clear()
	{
		m_CumulativeDamage.clear();
	}

	bool IsAlive() { return m_bIsAlive; }

private:
	std::map<int, unsigned int> m_CumulativeDamage;
	std::mutex m;

	bool m_bIsAlive;
	unsigned int m_MonsterCode;
	unsigned int m_HealthPoint;
	Vector2 m_position;
	Vector2 m_velocity;
};