#pragma once

#include "CharInfo.hpp"
#include <functional>

class User
{
public:
	User(int idx_) : m_idx(idx_)
	{
		m_mapIdx = -1;
		m_usercode = 0;
		m_charcode = 0;
		m_pCharInfo = nullptr;
	}

	void SetUserCode(int usercode_)
	{
		m_usercode = usercode_;
		return;
	}

	void SetCharCode(int charcode_)
	{
		m_charcode = charcode_;
		return;
	}

	void SetCharInfo()
	{
		
	}

	void Clear()
	{

	}

	int GetUserCode() const { return m_usercode; }

private:
	int m_idx;
	int m_mapIdx;
	int m_usercode;
	int m_charcode;
	CharInfo* m_pCharInfo;

	std::function<void()> ReleaseInfo;
};