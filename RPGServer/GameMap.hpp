#pragma once

#include <map>
#include <mutex>
#include <functional>
#include "PacketData.hpp"
#include "ItemObject.hpp"
#include "User.hpp"
#include <vector>
#include "Monster.hpp"

// �ʿ� �ʹ� ���� �������� ������ �ȵǴ� ���� �ð��� ���� �Ŀ� ������� ��������
// Priority Queue�� �̿��� <time_t, func>���� pair�� �����Ͽ� time_t�� ���� ������������ �����ϰ�
// Priority Queue�� front�� ��ȸ�Ͽ� ����ð����� ������ ����Ǿ���ϴ� ��ɵ��� pop�Ͽ� �����ϴ� ������ �ʿ��ϴ�.

namespace RPG
{
	const int MAX_MONSTER_COUNT_ON_MAP = 10;
	const int ITEM_LIFE_SEC = 60;

	class Map
	{
	public:
		Map(const int mapcode_) : m_mapcode(mapcode_) {};

		void Init(const int mapcode_)
		{
			m_mapcode = mapcode_;
			m_itemcnt = 0;
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

		// return true : killed monster -> drop item
		// false : monster survived.
		bool AttackMonster(const int connectionIdx_, const int Monsteridx_, const unsigned int damage_)
		{
			if (Monsteridx_ >= MAX_MONSTER_COUNT_ON_MAP || Monsteridx_ < 0)
			{
				return false;
			}

			Monster* target = m_Monsters[Monsteridx_];

			if (target == nullptr)
			{
				return false;
			}

			auto itr = users.find(connectionIdx_);

			if (itr == users.end())
			{
				return false;
			}

			int usercode = itr->second->GetUserCode();
			int iRet = target->GetDamaged(usercode, damage_);

			// ������ �ذ� ����

			// Heaviest Dealed Dealer
			if (iRet != 0)
			{
				// CreateObject ...
				// ���� ��� ����, ������Ʈ ���� ����
				return true;
			}
			return false;
		}

		void CreateObject(const int itemcode_, const int count_, const int owner_, Vector2& position_) // , const int x, const int y - position
		{
			std::lock_guard<std::mutex> guard(m_itemMutex);
			ItemObject* obj = AllocateItemObject();

			obj->Init(itemcode_, count_, owner_, position_);
			unsigned int cnt = m_itemcnt++;
			m_itemObjects.emplace(cnt, obj);

			ReserveJob(ITEM_LIFE_SEC, [this, cnt]() {DiscardObject(cnt); });

			// SendToAllUser() - Created Object Message

			return;
		}

		ItemObject* PopObject(const unsigned int objectNo_)
		{
			std::lock_guard<std::mutex> guard(m_itemMutex);
			auto itr = m_itemObjects.find(objectNo_);
			if (itr != m_itemObjects.end())
			{
				ItemObject* ret = itr->second;
				m_itemObjects.erase(objectNo_);
				return ret;
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
			}

			return;
		}

		//----- func pointer
		std::function<PacketData* ()> AllocatePacket;
		std::function<void(PacketData*)> DeallocatePacket;

		std::function<bool(PacketData*)> SendMsgFunc;
		
		std::function<ItemObject* ()> AllocateItemObject;
		std::function<void(ItemObject*)> DeallocateItemObject;

		// Ư�� �ð��� ����� �� Ư�� �Լ��� �����ش޶�� ����. (a,b) : a�� �Ŀ� b�Լ��� ����
		std::function<void(const time_t, const std::function<void()>&)> ReserveJob; 

	private:
		// �޽��� ��ü�� ���� ������ �ش� �Լ����� �ۼ����� ��� �ʿ�
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

		// concurrent_unordered_map ���� �;��µ� erase�� unsafe�ϴ�..
		std::map<int, User*> users; // connidx, userData 
		std::map<unsigned int, ItemObject*> m_itemObjects;

		Monster* m_Monsters[MAX_MONSTER_COUNT_ON_MAP];

		// C++14���� �о����� ���� �� �׷���..
		std::mutex m_userMutex;
		std::mutex m_itemMutex; 

		unsigned int m_itemcnt = 0;
		int m_mapcode;
	};
}