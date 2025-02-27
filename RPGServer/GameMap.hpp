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

// �ʿ� �ʹ� ���� �������� ������ �ȵǴ� ���� �ð��� ���� �Ŀ� ������� ��������
// Priority Queue�� �̿��� <time_t, func>���� pair�� �����Ͽ� time_t�� ���� ������������ �����ϰ�
// Priority Queue�� front�� ��ȸ�Ͽ� ����ð����� ������ ����Ǿ���ϴ� ��ɵ��� pop�Ͽ� �����ϴ� ������ �ʿ��ϴ�.

namespace RPG
{
	const int MAX_MONSTER_COUNT_ON_MAP = 10;
	const int MONSTER_BITMASK_END = 1024; // 2^10 -> 2^0~2^9���� ����
	const int ITEM_LIFE_SEC = 60;
	const int ITEM_OWNERSHIP_PERIOD_SEC = 20;
	const int SPAWN_INTERVAL_SEC = 7;
	const float DISTANCE_LIMIT_GET_OBJECT = 5.0f;

	class Map
	{
	public:
		Map(const int mapcode_) : m_mapcode(mapcode_), m_IsNew(true) {};

		void Init()
		{
			if (m_IsNew == false)
			{
				return;
			}

			m_IsNew = false;
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
			pUser_->SetMapCode(m_mapcode);
			std::cout << "GameMap[" << m_mapcode << "]::UserEnter : conn[" << connectionIdx_ << "] entered.\n";
			users.emplace(connectionIdx_, pUser_);
		}

		// �ϳ��� ��ȯ�ϴ� �ͺ��� �� ��ü�� ���͸� ��ȸ�ϸ� ��ȯ�ϵ��� �ϴ°� ������ ����.
		void CreateMonster()
		{
			bool bIsSpawned = false;

			for (size_t idx = 0; idx < MAX_MONSTER_COUNT_ON_MAP; idx++)
			{
				if (m_Monsters[idx].Spawn())
				{
					// ��ȯ ���� ����
					bIsSpawned = true;
				}
			}

			// ��ȯ�� ���Ͱ� ���ٸ� �ش� �ʿ� Spawn�� ���� ��ȸ�� ������ ����.
			// ��ȸ�� ���� ���� �ٽ� ��ȸ�� �����ϴ°� ������ ����������κ��� ���۵ȴ�.
			if (bIsSpawned)
			{
				m_NextSpawnTime = std::chrono::steady_clock::now() + std::chrono::seconds(SPAWN_INTERVAL_SEC);

				ReserveJob(SPAWN_INTERVAL_SEC, [this]() {CreateMonster(); });
			}
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

			User* pUser = GetUserInstanceByConnIdx(connectionIdx_);

			if (pUser == nullptr)
			{
				return std::pair<int, int>();
			}

			int charcode = pUser->GetCharCode();
			int iRet = target.GetDamaged(charcode, damage_);

			// Heaviest Dealed Dealer
			if (iRet != 0)
			{
				auto currentTime = std::chrono::steady_clock::now();

				// ��ȸ�� ����� ���̹Ƿ� �ٽ� ��ȸ�� �����Ѵ�.
				if (currentTime > m_NextSpawnTime.load())
				{
					m_NextSpawnTime = currentTime + std::chrono::seconds(SPAWN_INTERVAL_SEC);
					ReserveJob(SPAWN_INTERVAL_SEC, [this]() {CreateMonster(); });
				}
				
				return std::pair<int, int>(target.GetMonsterCode(),iRet);
			}

			return std::pair<int, int>(target.GetMonsterCode(), 0);
		}

		void CreateObject(const int itemcode_, const int count_, const int owner_, Vector2& position_) // , const int x, const int y - position
		{
			std::lock_guard<std::mutex> guard(m_itemMutex);
			ItemObject* obj = AllocateItemObject();

			time_t now = time(NULL);

			obj->Init(itemcode_, count_, owner_, position_, now + ITEM_OWNERSHIP_PERIOD_SEC);
			unsigned int cnt = m_itemcnt++;
			m_itemObjects.emplace(cnt, obj);

			NotifyObjectCreationToAll(cnt);

			ReserveJob(ITEM_LIFE_SEC, [this, cnt]() {DiscardObject(cnt); });

			return;
		}

		// charcode : 0 �� ��� ������ �Ҹ� ��û�̱� ������ �׳� �ش�.
		// ������ ���� ��û�� ��� ��ġ������ ���� �Է��Ѵ�.
		ItemObject* PopObject(const unsigned int objectNo_, const int charcode_ = 0, const Vector2& position_ = Vector2())
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

				// �Ÿ��� �� ���
				if (charcode_ != 0 && position_.SquaredDistance(ret->GetPosition()) > DISTANCE_LIMIT_GET_OBJECT)
				{
					return nullptr;
				}

				if (ret->CanGet(charcode_))
				{
					m_itemObjects.erase(objectNo_);
					return ret;
				}
			}
			return nullptr;
		}

		// ������ �������� ����ð��� ���� �Ŀ� ����ϴ� �Լ�
		// ������ ������ ��쿣 �翬�� nullptr�� ó���Ǿ� �Ѿ��.
		void DiscardObject(const unsigned int objectNo_)
		{
			ItemObject* obj = PopObject(objectNo_);

			if (obj != nullptr)
			{
				DeallocateItemObject(obj);
				// discard ����
			}

			return;
		}

		// ���� ���� �ʱ�ȭ�� �ʿ����� Ȯ���ϴ� �Լ�.
		bool IsNew() { return m_IsNew; }

		// ���� ���� �޽������� ����� ���� ��.
		void NotifyChatToAll(std::string& msg, int exceptUsercode = 0)
		{
			std::cout << "GameMap[" << m_mapcode << "]::NotifyChatToAll : Chat : " << msg << '\n';
			SendInfoToUsers(users, RESULTCODE::SEND_INFO_CHAT_EVERYONE, msg, exceptUsercode);
		}

		//----- func pointer
		std::function<void(std::map<int,User*>&, RESULTCODE, std::string&, int)> SendInfoToUsersFunc;
		std::function<void(const int, RESULTCODE, std::string&)> SendInfoFunc;
		
		std::function<ItemObject* ()> AllocateItemObject;
		std::function<void(ItemObject*)> DeallocateItemObject;

		// Ư�� �ð��� ����� �� Ư�� �Լ��� �����ش޶�� ����. (a,b) : a�� �Ŀ� b�Լ��� ����
		std::function<void(const long long, const std::function<void()>&)> ReserveJob; 

	private:
		User* GetUserInstanceByConnIdx(const int conn_idx_)
		{
			std::lock_guard<std::mutex> guard(m_userMutex);

			auto itr = users.find(conn_idx_);

			if (itr == users.end())
			{
				return nullptr;
			}

			return itr->second;
		}

		void NotifySpawnMonsterToAll(const int monsterIdx_)
		{
			MonsterSpawnResponse res{monsterIdx_};
			m_Monsters[monsterIdx_].GetInfo(res);

			std::string str;

			if (!m_jsonMaker.ToJsonString(res, str))
			{
				std::cerr << "GameMap::NotifySpawnMonsterToAll : Failed to Convert to json\n";
				return;
			}
			
			SendInfoToUsers(users, RESULTCODE::SEND_INFO_MONSTER_CREATED, str);
		}

		void NotifyMonsterGetDamagedToAll(MonsterGetDamageResponse& res_)
		{
			std::string str;

			if (!m_jsonMaker.ToJsonString(res_, str))
			{
				std::cerr << "GameMap::NotifyMonsterGetDamagedToAll : Failed to Convert to json\n";
				return;
			}

			SendInfoToUsers(users, RESULTCODE::SEND_INFO_MONSTER_GET_DAMAGE, str);
		}

		void NotifyDespawnMonsterToAll(const int monsterIdx_)
		{
			MonsterDespawnResponse res{monsterIdx_};

			std::string str;

			if (!m_jsonMaker.ToJsonString(res, str))
			{
				std::cerr << "GameMap::NotifyDespawnMonsterToAll : Failed to Convert to json\n";
				return;
			}

			SendInfoToUsers(users, RESULTCODE::SEND_INFO_MONSTER_DESPAWN, str);
		}

		void NotifyObjectCreationToAll(const unsigned int objectIdx_)
		{
			auto itr = m_itemObjects.find(objectIdx_);

			if (itr == m_itemObjects.end())
			{
				std::cerr << "GameMap::NotifyObjectCreationToAll : idx err\n";
				return;
			}

			CreateObjectResponse res{ objectIdx_ };

			if (itr->second == nullptr)
			{
				std::cerr << "GameMap::NotifyObjectCreationToAll : null ptr err\n";
			}

			itr->second->GetInfo(res);

			std::string str;

			if (!m_jsonMaker.ToJsonString(res, str))
			{
				std::cerr << "GameMap::NotifyObjectCreationToAll : Failed to Convert to json\n";
				return;
			}

			SendInfoToUsers(users, RESULTCODE::SEND_INFO_OBJECT_CREATED, str);
		}

		void NotifyObjectDiscardToAll(const unsigned int objectIdx_)
		{
			DiscardObjectResponse res{ objectIdx_ };

			std::string str;

			if (!m_jsonMaker.ToJsonString(res, str))
			{
				std::cerr << "GameMap::NotifyObjectDiscardToAll : Failed to Convert to json\n";
				return;
			}

			SendInfoToUsers(users, RESULTCODE::SEND_INFO_OBJECT_DISCARDED, str);
		}

		// ����ڴ� �����Ϸ� �ߴµ� �׳� Ŭ���̾�Ʈ���� ������ ��
		void NotifyObjectObtainedToAll(const unsigned int objectIdx_, const int charCode_)
		{
			ObtainObjectResponse res{ objectIdx_, charCode_ };
			std::string str;
			
			if (!m_jsonMaker.ToJsonString(res, str))
			{
				std::cerr << "GameMap::NotifyObjectObtainedToAll : Failed to Convert to json\n";
				return;
			}

			SendInfoToUsers(users, RESULTCODE::SEND_INFO_OBJECT_OBTAINED, str);
		}

		void SendInfoToUsers(std::map<int, User*>& users, RESULTCODE rescode, std::string& msg, int exceptUsercode = 0)
		{
			SendInfoToUsersFunc(users, rescode, msg, exceptUsercode);
		}

		std::map<int, User*> users; // connidx, userData 
		std::map<unsigned int, ItemObject*> m_itemObjects;

		Monster m_Monsters[MAX_MONSTER_COUNT_ON_MAP];

		std::mutex m_userMutex;
		std::mutex m_itemMutex; 

		std::atomic<unsigned int> m_itemcnt = 0;
		std::atomic<int> m_mapcode;
		std::atomic<bool> m_IsNew;
		std::atomic<std::chrono::steady_clock::time_point> m_NextSpawnTime;

		// �̰� �������� �ν��Ͻ� ũ�� Ŀ�� �̱������� ���°� �������� ���� �� ����.
		JsonMaker m_jsonMaker;
	};
}