#pragma once

#include "User.hpp"
#include <vector>

class UserManager
{
public:
	~UserManager()
	{
		Clear();
	}

	void Init(const int maxCount_)
	{
		users.clear();
		users.reserve(maxCount_);
		for (int i = 0; i < maxCount_; i++)
		{
			users.push_back(new User(i));
			users[i]->ReleaseInfo = this->ReleaseInfo;
		}
	}

	// 리턴값을 null체크 하고 반환할것.
	CharInfo* ReleaseUser(const int connectionIdx_)
	{
		return users[connectionIdx_]->Clear();
	}

	void Clear()
	{
		for (auto* i : users)
		{
			if (i != nullptr)
			{
				delete i;
				i = nullptr;
			}
		}

		users.clear();
	}

	User* GetUserByConnIndex(const int connectionIndex_) { return users[connectionIndex_]; }

	std::function<void(CharInfo*)> ReleaseInfo;

private:
	std::vector<User*> users;
	std::mutex m;
};