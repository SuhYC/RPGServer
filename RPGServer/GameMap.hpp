#pragma once

#include <set>
#include <mutex>
#include <functional>
#include "PacketData.hpp"

namespace RPG
{
	class Map
	{
	public:
		Map(const int mapcode_) : m_mapcode(mapcode_) {};

		void Init(const int mapcode_)
		{
			m_mapcode = mapcode_;
		}

		void UserExit(const int connectionIdx_)
		{
			std::lock_guard<std::mutex> guard(m);
			users.erase(connectionIdx_);
		}

		void UserEnter(const int connectionIdx_)
		{
			std::lock_guard<std::mutex> guard(m);
			users.insert(connectionIdx_);
		}

		std::function<bool(PacketData*)> SendPacketFunc;
		std::function<PacketData* ()> AllocatePacket;

	private:

		// 매개변수 수정할것.
		void SendToAllUser(bool exceptme)
		{
			PacketData* tmp = AllocatePacket();

			// 패킷 작성할것

			for (auto i : users)
			{
				PacketData* pPacket = AllocatePacket();
				pPacket->Init(tmp, i);
				SendPacketFunc(tmp);
			}
		}

		std::set<int> users; // concurrent_unordered_set 쓰고 싶었는데 erase가 unsafe하다..
		std::mutex m; // C++14에서 읽쓰락을 쓰긴 좀 그러니..
		int m_mapcode;
	};
}