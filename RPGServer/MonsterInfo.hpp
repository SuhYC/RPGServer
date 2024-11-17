#pragma once

#include "Vector.hpp"

/*
* DB_SCHEMA
* 
* 이 정보가 MAP에 기록된다.
* T(SPAWNINFO) [MAPCODE] [MONSTERCODE] [POSX] [POSY] // 맵의 어느좌표에 어느 몬스터를 소환할지 
* 
* MAXHEALTH까지만 가져간다. 나머지는 어차피 로직상 ReqHandler에서 DB조회하고 가져올 수 있다. (정확히는 DB를 조회해서 기록해둔 UMAP.)
* T(MONSTERINFO) [MONSTERCODE] [MAXHEALTH] [EXPERIENCE] [DROPGOLD] // 해당 몬스터의 체력, 사냥시 경험치, 사냥시 떨어지는 골드량
* 
* T(DROPINFO) [MONSTERCODE] [ITEMCODE] [RATIO] // 해당 몬스터가 확률적으로 떨어뜨리는 아이템정보
*/

class MonsterSpawnInfo
{
public:
	MonsterSpawnInfo(const int monsterCode_, const int maxHealth_, Vector2& SpawnPosition_) noexcept
		: m_MonsterCode(monsterCode_), m_MaxHealth(maxHealth_), m_SpawnPosition(SpawnPosition_)
	{

	}

	MonsterSpawnInfo(const int monsterCode_, const int maxHealth_, Vector2&& SpawnPosition_) noexcept
		: m_MonsterCode(monsterCode_), m_MaxHealth(maxHealth_), m_SpawnPosition(SpawnPosition_)
	{

	}

	MonsterSpawnInfo(const MonsterSpawnInfo& other_) noexcept
	{
		m_SpawnPosition = other_.m_SpawnPosition;
		m_MaxHealth = other_.m_MaxHealth;
		m_MonsterCode = other_.m_MonsterCode;
	}

	MonsterSpawnInfo(const MonsterSpawnInfo&& other_) noexcept
	{
		m_SpawnPosition = other_.m_SpawnPosition;
		m_MaxHealth = other_.m_MaxHealth;
		m_MonsterCode = other_.m_MonsterCode;
	}

	void Init(const int monsterCode_, const int maxHealth_, Vector2& SpawnPosition_)
	{
		m_MonsterCode = monsterCode_;
		m_MaxHealth = maxHealth_;
		m_SpawnPosition = SpawnPosition_;
	}

	void Init(const int monsterCode_, const int maxHealth_, Vector2&& SpawnPosition_)
	{
		m_MonsterCode = monsterCode_;
		m_MaxHealth = maxHealth_;
		m_SpawnPosition = SpawnPosition_;
	}

	Vector2 m_SpawnPosition;
	int m_MaxHealth;
	int m_MonsterCode;
};
