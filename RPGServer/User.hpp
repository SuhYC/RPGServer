#pragma once

#include "CharInfo.hpp"
#include "Vector.hpp"
#include <functional>
#include <iostream>
#include <chrono>
#include <mutex>
#include <atomic>

const int RESERVE_NICKNAME_TIME_SEC = 5 * 60;

class User
{
public:
	User(const int idx_) : m_idx(idx_)
	{
		Clear();
		m_LastUpdateTime = std::chrono::steady_clock::now();
	}

	~User()
	{
		End();
	}

	void SetIP(const uint32_t ip_)
	{
		m_ip.store(ip_);
		m_IsConnected.store(true);
		return;
	}

	void SetUserCode(const int usercode_)
	{
		m_usercode.store(usercode_);
		return;
	}

	void UpdatePosition(Vector2& position_, Vector2& velocity_)
	{
		m_LastUpdateTime = std::chrono::steady_clock::now();
		m_stPosition.store(position_);
		m_stVelocity.store(velocity_);
	}

	void SetCharInfo(CharInfo* pInfo_)
	{
		std::lock_guard<std::mutex> guard(m_mutex);
		if (m_pCharInfo != nullptr)
		{
			ReleaseInfo(m_pCharInfo);
		}
		m_pCharInfo = pInfo_;

		m_mapIdx = m_pCharInfo->LastMapCode;
		m_charcode = m_pCharInfo->CharNo;
	}

	void SetMapCode(const int mapCode_)
	{
		m_mapIdx.store(mapCode_);
		return;
	}

	// CharInfo 객체를 반환한다. 해당 정보를 DB에 기록하고 반환할것.
	// memory order를 적용하면 더 좋은 코드가 될 것 같다.
	CharInfo* Clear()
	{
		std::lock_guard<std::mutex> guard(m_mutex);
		if (m_pCharInfo == nullptr)
		{
			return nullptr;
		}

		m_IsConnected = false;

		m_mapIdx = -1;
		m_usercode = 0;
		m_charcode = 0;
		m_ip.store(0);

		CharInfo* ret = m_pCharInfo;
		m_pCharInfo = nullptr;
		return ret;
	}

	// 서버 가동 중단 등으로 메모리를 풀에 반납하지 않고 해제하는 함수이다.
	void End()
	{
		if (m_pCharInfo != nullptr)
		{
			delete m_pCharInfo;
		}
	}

	int GetCharCode() const noexcept { return m_charcode.load(); }
	int GetUserCode() const noexcept { return m_usercode.load(); }
	int GetMapCode() const noexcept { return m_mapIdx.load(); }
	bool IsConnected() const noexcept { return m_IsConnected.load(); }

	// 바이트형식의 ip를 std::string으로 변환
	const std::string& GetIP() const noexcept
	{
		char ipStr[INET_ADDRSTRLEN];

		if (inet_ntop(AF_INET, &m_ip, ipStr, sizeof(ipStr)) == nullptr)
		{
			return std::string();
		}

		return std::string(ipStr);
	}
	
	std::string GetReserveNickname() noexcept
	{
		auto currentTime = std::chrono::steady_clock::now();

		// 유효기간이 지남.
		if (currentTime > m_ValidTimeOfReserveNickname.load())
		{
			m_reserveNickname = "";
			return std::string();
		}

		return m_reserveNickname;
	}

	void SetReserveNickname(std::string& nickname_)
	{
		std::lock_guard<std::mutex> guard(m_mutex);
		m_reserveNickname = nickname_;
		m_ValidTimeOfReserveNickname = std::chrono::steady_clock::now() + std::chrono::seconds(RESERVE_NICKNAME_TIME_SEC);
	}

	// 데드레커닝.
	// 위치와 속도를 이용해 마지막 업데이트로부터 시간차를 이용해 위치를 계산한다.
	Vector2 GetPosition() const noexcept
	{
		auto now = std::chrono::steady_clock::now();
		auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastUpdateTime.load());
		int count = elapsedTime.count();

		return m_stPosition.load() + (m_stVelocity.load() * count);
	}

	Vector2 GetVelocity() const noexcept { return m_stVelocity.load(); }

	long long calStatDamage() const
	{
		if (m_pCharInfo == nullptr)
		{
			return 0;
		}

		// --작성 필요

		return 1;
	}

	std::function<void(CharInfo*)> ReleaseInfo;

private:
	std::mutex m_mutex;

	int m_idx;
	std::atomic_int m_mapIdx;
	std::atomic_int m_usercode;
	std::atomic_int m_charcode;
	std::atomic<uint32_t> m_ip;
	CharInfo* m_pCharInfo;

	std::atomic<Vector2> m_stPosition;
	std::atomic<Vector2> m_stVelocity; // delta(position) / ms로 할것
	std::atomic<std::chrono::steady_clock::time_point> m_LastUpdateTime;

	std::atomic_bool m_IsConnected;

	std::string m_reserveNickname;
	std::atomic<std::chrono::steady_clock::time_point> m_ValidTimeOfReserveNickname;
};