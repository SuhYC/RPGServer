#pragma once

#include "User.hpp"
#include <vector>

class UserManager
{
public:
	void Init(const int maxCount_)
	{
		users.reserve(maxCount_);
		for (int i = 0; i < maxCount_; i++)
		{
			users.push_back(new User(i));
		}
	}

	void AddUser(const int connection_idx_, CharInfo* pInfo_)
	{

	}

private:
	std::vector<User*> users;
};