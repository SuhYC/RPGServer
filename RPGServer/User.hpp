#pragma once

#include "CharInfo.hpp"
#include "Vector.hpp"
#include <functional>
#include <iostream>
#include <chrono>

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

	void SetIP(const std::string& ip_)
	{
		m_ip.assign(ip_);
		m_IsConnected = true;
		return;
	}

	void SetUserCode(const int usercode_)
	{
		m_usercode = usercode_;
		return;
	}

	void UpdatePosition(Vector2& position_, Vector2& velocity_)
	{
		m_LastUpdateTime = std::chrono::steady_clock::now();
		m_stPosition = position_;
		m_stVelocity = velocity_;
	}

	void SetCharInfo(CharInfo* pInfo_)
	{
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
		m_mapIdx = mapCode_;
		return;
	}

	// CharInfo ��ü�� ��ȯ�Ѵ�. �ش� ������ DB�� ����ϰ� ��ȯ�Ұ�.
	CharInfo* Clear()
	{
		if (m_IsConnected == false)
		{
			return nullptr;
		}

		m_IsConnected = false;

		m_mapIdx = -1;
		m_usercode = 0;
		m_charcode = 0;
		m_ip.clear();

		CharInfo* ret = m_pCharInfo;
		m_pCharInfo = nullptr;
		return ret;
	}

	// ���� ���� �ߴ� ������ �޸𸮸� Ǯ�� �ݳ����� �ʰ� �����ϴ� �Լ��̴�.
	void End()
	{
		if (m_pCharInfo != nullptr)
		{
			delete m_pCharInfo;
		}
	}

	int GetCharCode() const noexcept { return m_charcode; }
	int GetUserCode() const noexcept { return m_usercode; }
	int GetMapCode() const noexcept { return m_mapIdx; }
	bool IsConnected() const noexcept { return m_IsConnected; }
	const std::string& GetIP() const noexcept { return m_ip; }

	// ���巹Ŀ��.
	// ��ġ�� �ӵ��� �̿��� ������ ������Ʈ�κ��� �ð����� �̿��� ��ġ�� ����Ѵ�.
	Vector2 GetPosition() const noexcept
	{
		auto now = std::chrono::steady_clock::now();
		auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastUpdateTime);
		int count = elapsedTime.count();

		return m_stPosition + (m_stVelocity * count);
	}

	Vector2 GetVelocity() const noexcept { return m_stVelocity; }

	std::function<void(CharInfo*)> ReleaseInfo;

private:
	int m_idx;
	int m_mapIdx;
	int m_usercode;
	int m_charcode;
	std::string m_ip;
	CharInfo* m_pCharInfo;

	Vector2 m_stPosition;
	Vector2 m_stVelocity; // delta(position) / ms�� �Ұ�
	std::chrono::steady_clock::time_point m_LastUpdateTime;

	bool m_IsConnected;
};