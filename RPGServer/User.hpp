#pragma once

#include "CharInfo.hpp"
#include <functional>
#include <iostream>

class User
{
public:
	User(const int idx_) : m_idx(idx_)
	{
		Clear();
	}

	~User()
	{
		End();
	}

	void SetIP(std::string ip_)
	{
		m_ip.assign(ip_);

		return;
	}

	void SetUserCode(const int usercode_)
	{
		m_usercode = usercode_;
		return;
	}

	void SetCharCode(const int charcode_)
	{
		m_charcode = charcode_;
		return;
	}

	void SetCharInfo(CharInfo* pInfo_)
	{
		if (m_pCharInfo != nullptr)
		{
			ReleaseInfo(m_pCharInfo);
		}
		m_pCharInfo = pInfo_;

		m_mapIdx = m_pCharInfo->LastMapCode;
	}

	void SetMapCode(const int mapCode_)
	{
		m_mapIdx = mapCode_;
		return;
	}

	void Clear()
	{
		if (m_IsConnected == false)
		{
			return;
		}

		m_IsConnected = false;

		m_mapIdx = -1;
		m_usercode = 0;
		m_charcode = 0;
		m_ip.clear();

		if (m_pCharInfo != nullptr)
		{
			ReleaseInfo(m_pCharInfo);
			m_pCharInfo = nullptr;
		}
	}

	// 서버 가동 중단 등으로 메모리를 풀에 반납하지 않고 해제하는 함수이다.
	void End()
	{
		if (m_pCharInfo != nullptr)
		{
			delete m_pCharInfo;
		}
	}

	int GetUserCode() const { return m_usercode; }
	bool IsConnected() const { return m_IsConnected; }
	const std::string& GetIP() const { return m_ip; }

	std::function<void(CharInfo*)> ReleaseInfo;

private:
	int m_idx;
	int m_mapIdx;
	int m_usercode;
	int m_charcode;
	std::string m_ip;
	CharInfo* m_pCharInfo;

	bool m_IsConnected;
};