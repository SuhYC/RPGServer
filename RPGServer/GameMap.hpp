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

		// �Ű����� �����Ұ�.
		void SendToAllUser(bool exceptme)
		{
			PacketData* tmp = AllocatePacket();

			// ��Ŷ �ۼ��Ұ�

			for (auto i : users)
			{
				PacketData* pPacket = AllocatePacket();
				pPacket->Init(tmp, i);
				SendPacketFunc(tmp);
			}
		}

		std::set<int> users; // concurrent_unordered_set ���� �;��µ� erase�� unsafe�ϴ�..
		std::mutex m; // C++14���� �о����� ���� �� �׷���..
		int m_mapcode;
	};
}