#pragma once

#include <map>
#include <mutex>
#include <functional>
#include "PacketData.hpp"
#include "ItemObject.hpp"
#include "User.hpp"
#include <vector>
#include "Monster.hpp"
#include "Vector.hpp"
#include "MonsterInfo.hpp"

// 맵에 너무 많은 아이템이 있으면 안되니 일정 시간이 지난 후에 사라지게 만들어야함
// Priority Queue를 이용해 <time_t, func>으로 pair를 구성하여 time_t에 대해 오름차순으로 정렬하고
// Priority Queue의 front를 조회하여 현재시간보다 이전에 실행되어야하는 명령들을 pop하여 실행하는 구조가 필요하다.

namespace RPG
{
	const int MAX_MONSTER_COUNT_ON_MAP = 10;
	const int MONSTER_BITMASK_END = 1024; // 2^10 -> 2^0~2^9까지 가능
	const int ITEM_LIFE_SEC = 60;
	const int ITEM_OWNERSHIP_PERIOD = 20;
	const int SPAWN_INTERVAL_SEC = 7;
	const float DISTANCE_LIMIT_GET_OBJECT = 5.0f;

	class Map
	{
	public:
		Map(const int mapcode_) : m_mapcode(mapcode_) {};

		void Init(const int monstercode_, const int monstercount_)
		{
			m_itemcnt = 0;
			m_NextSpawnTime = std::chrono::steady_clock::now() + std::chrono::seconds(SPAWN_INTERVAL_SEC);
		}

		void UserExit(const int connectionIdx_)
		{
			std::lock_guard<std::mutex> guard(m_userMutex);
			users.erase(connectionIdx_);
		}

		void UserEnter(const int connectionIdx_, User* pUser_)
		{
			std::lock_guard<std::mutex> guard(m_userMutex);
			users.emplace(connectionIdx_, pUser_);
		}

		void CreateMonster(const int monsteridx_)
		{
			auto currentTime = std::chrono::steady_clock::now();

			if (currentTime > m_NextSpawnTime)
			{
				m_NextSpawnTime += std::chrono::seconds(SPAWN_INTERVAL_SEC);
			}

			m_Monsters[monsteridx_].Spawn();
		}

		void InitSpawnInfo(const int monsteridx_, const MonsterSpawnInfo& info_)
		{
			m_Monsters[monsteridx_].Init(info_.m_MonsterCode, info_.m_MaxHealth, info_.m_SpawnPosition);
		}

		// ret : std::pair<monstercode, charcode>
		// charcode : heaviest dealed dealer's code (ownership)
		// std::pair<0, ?> : no hit
		// std::pair<monstercode, 0> : getDamaged
		// std::pair<monstercode, charcode> : killed monster
		std::pair<int, int> AttackMonster(const int connectionIdx_, const int Monsteridx_, const unsigned int damage_)
		{
			if (Monsteridx_ >= MAX_MONSTER_COUNT_ON_MAP || Monsteridx_ < 0)
			{
				return std::pair<int, int>();
			}

			Monster& target = m_Monsters[Monsteridx_];

			if (target.IsAlive() == false)
			{
				return std::pair<int, int>();
			}

			auto itr = users.find(connectionIdx_);

			if (itr == users.end())
			{
				return std::pair<int, int>();
			}

			int charcode = itr->second->GetCharCode();
			int iRet = target.GetDamaged(charcode, damage_);

			// Heaviest Dealed Dealer
			if (iRet != 0)
			{
				auto currentTime = std::chrono::steady_clock::now();

				ReserveJob(std::chrono::duration_cast<std::chrono::seconds>(m_NextSpawnTime - currentTime).count(), [this, Monsteridx_]() {CreateMonster(Monsteridx_); });

				return std::pair<int, int>(target.GetMonsterCode(),iRet);
			}

			return std::pair<int, int>(target.GetMonsterCode(), 0);
		}

		void CreateObject(const int itemcode_, const int count_, const int owner_, Vector2& position_) // , const int x, const int y - position
		{
			std::lock_guard<std::mutex> guard(m_itemMutex);
			ItemObject* obj = AllocateItemObject();

			time_t now = time(NULL);

			obj->Init(itemcode_, count_, owner_, position_, now + ITEM_OWNERSHIP_PERIOD);
			unsigned int cnt = m_itemcnt++;
			m_itemObjects.emplace(cnt, obj);

			ReserveJob(ITEM_LIFE_SEC, [this, cnt]() {DiscardObject(cnt); });

			return;
		}

		// usercode : 0 인 경우 아이템 소멸 요청이기 때문에 그냥 준다.
		// 유저의 습득 요청의 경우 위치정보도 같이 입력한다.
		ItemObject* PopObject(const unsigned int objectNo_, const int usercode_ = 0, const Vector2& position_ = Vector2())
		{
			std::lock_guard<std::mutex> guard(m_itemMutex);
			auto itr = m_itemObjects.find(objectNo_);
			if (itr != m_itemObjects.end())
			{
				ItemObject* ret = itr->second;

				if (ret == nullptr)
				{
					return nullptr;
				}

				// 거리가 먼 경우
				if (usercode_ != 0 && position_.SquaredDistance(ret->GetPosition()) > DISTANCE_LIMIT_GET_OBJECT)
				{
					return nullptr;
				}

				if (ret->CanGet(usercode_))
				{
					m_itemObjects.erase(objectNo_);
					return ret;
				}
			}
			return nullptr;
		}

		// 생성된 아이템을 만료시간이 지난 후에 폐기하는 함수
		// 유저가 습득한 경우엔 당연히 nullptr로 처리되어 넘어간다.
		void DiscardObject(const unsigned int objectNo_)
		{
			ItemObject* obj = PopObject(objectNo_);

			if (obj != nullptr)
			{
				DeallocateItemObject(obj);
			}

			return;
		}

		// 생성 직후 초기화가 필요한지 확인하는 함수.
		bool IsNew() { return std::chrono::duration_cast<std::chrono::seconds>(m_NextSpawnTime.time_since_epoch()).count() == 0; }

		//----- func pointer
		std::function<PacketData* ()> AllocatePacket;
		std::function<void(PacketData*)> DeallocatePacket;

		std::function<bool(PacketData*)> SendMsgFunc;
		
		std::function<ItemObject* ()> AllocateItemObject;
		std::function<void(ItemObject*)> DeallocateItemObject;

		// 특정 시간이 경과된 후 특정 함수를 실행해달라고 예약. (a,b) : a초 후에 b함수를 실행
		std::function<void(const long long, const std::function<void()>&)> ReserveJob; 

		// 메시지 주체에 대한 정보를 해당 함수에서 작성할지 고민 필요
		void SendToAllUser(std::string& data_, const int connectionIdx_, bool bExceptme_)
		{
			PacketData* pTmpPacket = AllocatePacket();
			if (pTmpPacket == nullptr)
			{
				std::cerr << "GameMap::SendToAllUser : Failed to Create Packet On Map No" << m_mapcode << '\n';
				return;
			}
			pTmpPacket->Init(0, data_);

			for (auto& i : users)
			{
				if (bExceptme_ && i.first == connectionIdx_)
				{
					continue;
				}

				PacketData* pPacket = AllocatePacket();

				if (pPacket == nullptr)
				{
					DeallocatePacket(pTmpPacket);
					std::cerr << "GameMap::SendToAllUser : Failed to Create Packet On Map No" << m_mapcode << "\n";
					return;
				}

				pPacket->Init(pTmpPacket, i.first);
				SendMsgFunc(pPacket);
			}

			DeallocatePacket(pTmpPacket);

			return;
		}
	private:
		

		// concurrent_unordered_map 쓰고 싶었는데 erase가 unsafe하다..
		std::map<int, User*> users; // connidx, userData 
		std::map<unsigned int, ItemObject*> m_itemObjects;

		Monster m_Monsters[MAX_MONSTER_COUNT_ON_MAP];

		// C++14에서 읽쓰락을 쓰긴 좀 그러니..
		std::mutex m_userMutex;
		std::mutex m_itemMutex; 

		unsigned int m_itemcnt = 0;
		int m_mapcode;
		std::chrono::steady_clock::time_point m_NextSpawnTime;
	};
}