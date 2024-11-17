#pragma once

#include "Vector.hpp"

/*
* DB_SCHEMA
* 
* �� ������ MAP�� ��ϵȴ�.
* T(SPAWNINFO) [MAPCODE] [MONSTERCODE] [POSX] [POSY] // ���� �����ǥ�� ��� ���͸� ��ȯ���� 
* 
* MAXHEALTH������ ��������. �������� ������ ������ ReqHandler���� DB��ȸ�ϰ� ������ �� �ִ�. (��Ȯ���� DB�� ��ȸ�ؼ� ����ص� UMAP.)
* T(MONSTERINFO) [MONSTERCODE] [MAXHEALTH] [EXPERIENCE] [DROPGOLD] // �ش� ������ ü��, ��ɽ� ����ġ, ��ɽ� �������� ��差
* 
* T(DROPINFO) [MONSTERCODE] [ITEMCODE] [RATIO] // �ش� ���Ͱ� Ȯ�������� ����߸��� ����������
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
