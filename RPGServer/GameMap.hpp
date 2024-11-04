#pragma once

#include <map>
#include <mutex>
#include <functional>
#include "PacketData.hpp"
#include "ItemObject.hpp"
#include "User.hpp"
#include <vector>
#include "Monster.hpp"

namespace RPG
{
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
			if (Monsteridx_ >= 10 || Monsteridx_ < 0)
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

			// Heaviest Dealed Dealer
			if (iRet != 0)
			{
				// CreateObject ...
				return true;
			}
			return false;
		}

		void CreateObject(const int itemcode_, const int count_, const int owner_, Vector2& position_) // , const int x, const int y - position
		{
			std::lock_guard<std::mutex> guard(m_itemMutex);
			ItemObject* obj = AllocateItemObject();

			obj->Init(itemcode_, count_, owner_, position_);

			m_itemObjects.emplace(m_itemcnt++, obj);

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


		//----- func pointer
		std::function<bool(const unsigned short, const std::string&, PacketData* const)> MakePacketFunc;
		std::function<PacketData* ()> AllocatePacket;
		std::function<void(PacketData*)> DeallocatePacket;
		std::function<bool(PacketData*)> SendMsgFunc;
		
		std::function<ItemObject* ()> AllocateItemObject;
		
	private:
		// 메시지 주체에 대한 정보를 해당 함수에서 작성할지 고민 필요
		void SendToAllUser(std::string& data_, const int connectionIdx_, bool bExceptme_)
		{
			PacketData* pTmpPacket = AllocatePacket();
			if (!MakePacketFunc(0, data_, pTmpPacket))
			{
				DeallocatePacket(pTmpPacket);
				std::cerr << "GameMap::SendToAllUser : Failed to Create Packet On Map No" << m_mapcode << "\n";
				return;
			}

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

		// concurrent_unordered_map 쓰고 싶었는데 erase가 unsafe하다..
		std::map<int, User*> users; // connidx, userData 
		std::map<unsigned int, ItemObject*> m_itemObjects;

		Monster* m_Monsters[10];

		// C++14에서 읽쓰락을 쓰긴 좀 그러니..
		std::mutex m_userMutex;
		std::mutex m_itemMutex; 

		unsigned int m_itemcnt = 0;
		int m_mapcode;
	};
}