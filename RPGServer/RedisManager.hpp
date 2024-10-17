#pragma once

#include "PacketData.hpp"
#include "CharInfo.hpp"
#include "RedisConnectionPool.hpp"
#include "Json.hpp"

class RedisManager
{
public:
	RedisManager()
	{
		m_ConnectionPool = std::make_unique<Redis::ConnectionPool>();
	}

	virtual ~RedisManager()
	{
		m_ConnectionPool.reset();
	}

	// key : usercode, value : ip?
	bool MakeSession(const int usercode_, const int ip_)
	{
		if (SetNx(usercode_, ip_))
		{
			return true;
		}
		return false;
	}

	// Create, Renew Info of Char on Redis(Cache)
	bool UpdateCharInfo(const int charNo_, CharInfo* pInfo_)
	{
		if (pInfo_ == nullptr)
		{
			return false;
		}

		std::wstring key = std::to_wstring(charNo_) + L"CharInfo";
		std::string value;
		m_jsonMaker.ToJsonString(pInfo_, value);

		if (Set(key, value))
		{
			return true;
		}
		return false;
	}

	bool GetCharInfo(const int charNo_, CharInfo* out_)
	{
		if (out_ == nullptr)
		{
			return false;
		}

		std::wstring key = std::to_wstring(charNo_) + L"CharInfo";
		std::string value;

		if (Get(key, value) == false)
		{
			return false;
		}

		if (m_jsonMaker.ToCharInfo(value, *out_))
		{
			return true;
		}

		return false;
	}

	void LogOut(const int userCode_)
	{
		Del(userCode_);

		return;
	}

private:
	// str(usercode+datatype) : key_
	// jsonstr : value_
	bool Set(const std::wstring& key_, const std::string& value_)
	{
		redisContext* rc = AllocateConnection();
		redisReply* reply = reinterpret_cast<redisReply*>(redisCommand(rc, "SET %s %s", key_.c_str(), value_.c_str()));

		DeallocateConnection(rc);
		if (reply != nullptr)
		{
			if (reply->type == REDIS_REPLY_STATUS)
			{
				if (std::string(reply->str) == "OK")
				{
					freeReplyObject(reply);
					return true;
				}
				else
				{
					freeReplyObject(reply);
					return false;
				}
			}
			else
			{
				std::cerr << "RedisManager::Set : Not Status reply.\n";
			}
		}

		freeReplyObject(reply);
		return false;
	}

	// for make session
	bool SetNx(const int key_, const int value_)
	{
		redisContext* rc = AllocateConnection();
		redisReply* reply = reinterpret_cast<redisReply*>(redisCommand(rc, "SETNX %d %d", key_, value_));

		DeallocateConnection(rc);
		if (reply != nullptr)
		{
			if (reply->type == REDIS_REPLY_INTEGER)
			{
				if (reply->integer == 1)
				{
					freeReplyObject(reply);
					return true;
				}
				else
				{
					freeReplyObject(reply);
					return false;
				}
			}
			else
			{
				std::cerr << "RedisManager::SetNx : Not integer reply.\n";
			}
		}

		freeReplyObject(reply);
		return false;
	}

	bool Del(const std::wstring& key_)
	{
		redisContext* rc = AllocateConnection();
		redisReply* reply = reinterpret_cast<redisReply*>(redisCommand(rc, "DEL %s", key_.c_str()));
		DeallocateConnection(rc);

		if (reply != nullptr)
		{
			if (reply->type == REDIS_REPLY_INTEGER)
			{
				if (reply->integer == 1)
				{
					freeReplyObject(reply);
					return true;
				}
				else
				{
					freeReplyObject(reply);
					return false;
				}
			}
		}

		freeReplyObject(reply);
		return false;
	}

	// for logout
	bool Del(const int usercode_)
	{
		redisContext* rc = AllocateConnection();
		redisReply* reply = reinterpret_cast<redisReply*>(redisCommand(rc, "DEL %d", usercode_));
		DeallocateConnection(rc);

		if (reply != nullptr)
		{
			if (reply->type == REDIS_REPLY_INTEGER)
			{
				if (reply->integer == 1)
				{
					freeReplyObject(reply);
					return true;
				}
				else
				{
					freeReplyObject(reply);
					return false;
				}
			}
		}

		freeReplyObject(reply);
		return false;
	}

	bool Get(const std::wstring& key_, std::string& out_)
	{
		redisContext* rc = AllocateConnection();
		redisReply* reply = reinterpret_cast<redisReply*>(redisCommand(rc, "GET %s", key_.c_str()));
		DeallocateConnection(rc);

		if (reply != nullptr)
		{
			if (reply->type == REDIS_REPLY_STRING)
			{
				out_ = std::string(reply->str, reply->len);

				freeReplyObject(reply);
				return true;
			}
		}

		freeReplyObject(reply);
		return false;
	}

	bool Get(const int key_, std::string& out_)
	{
		redisContext* rc = AllocateConnection();
		redisReply* reply = reinterpret_cast<redisReply*>(redisCommand(rc, "GET %d", key_));
		DeallocateConnection(rc);

		if (reply != nullptr)
		{
			if (reply->type == REDIS_REPLY_STRING)
			{
				out_ = std::string(reply->str, reply->len);

				freeReplyObject(reply);
				return true;
			}
		}

		freeReplyObject(reply);
		return false;
	}

	redisContext* AllocateConnection()
	{
		redisContext* ret = m_ConnectionPool->Allocate();

		return ret;
	}

	void DeallocateConnection(redisContext* pRC_)
	{
		m_ConnectionPool->Deallocate(pRC_);

		return;
	}

	std::unique_ptr<Redis::ConnectionPool> m_ConnectionPool;
	JsonMaker m_jsonMaker;
};