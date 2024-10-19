#pragma once

#include "PacketData.hpp"
#include "CharInfo.hpp"
#include "RedisConnectionPool.hpp"
#include "Json.hpp"

/*
* ���𽺿� ������ Ű�� ������ ���� ��Ģ�� ������.
* 1. int�� ����
*  - �α��ε� ������ ������ȣ�� �ǹ��Ѵ�. value�� ip������ ���� ����
* 2. int�� ���� + �ν��Ͻ��̸�(std::wstring)
*  - Ư��ĳ������ ������ ��� ����ü�� Jsonȭ �Ͽ� value�� ����.
* 3. L"lock" + int�� ���� + �ν��Ͻ��̸�(std::wstring)
*  - 2�� �����͸� �����ϱ� ���� ���𽺺л��. ��� �� �ݵ�� �����Ͽ����Ѵ�.
*  - �л���� �ɰ� �����ϱ� ���� ������ �ٿ�Ǹ� ū���̴�.
*  - ��ȿ�Ⱓ�� �ɾ�߰ڴ�. 10�� ������ �ұ�?
* 
* ���� �л��
* 1. �б⿡�� ���� �ʴ´�.
* 2. �ʱ� ���ε忡�� ���� �ʴ´�.
* 3. �ʱ� ���ε� ���� ��� ���ۿ� ���� �������� ����.
*  ex) BuyItem(const int charno, const int price) -> must use lock
*/

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
	bool MakeSession(const int usercode_, const std::wstring& ip_)
	{
		if (SetNx(usercode_, ip_))
		{
			return true;
		}
		return false;
	}

	// Create Info of Char on Redis(Cache)
	// data must not be on redis
	bool CreateCharInfo(const int charNo_, CharInfo* pInfo_)
	{
		if (pInfo_ == nullptr)
		{
			return false;
		}

		std::wstring key = std::to_wstring(charNo_) + L"CharInfo";
		std::string value;
		m_jsonMaker.ToJsonString(pInfo_, value);

		if (SetNx(key, value))
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
	// for create data
	// wstr(usercode+datatype) : key_
	// jsonstr : value_
	bool SetNx(const std::wstring& key_, const std::string& value_)
	{
		redisContext* rc = AllocateConnection();
		redisReply* reply = reinterpret_cast<redisReply*>(redisCommand(rc, "SETNX %s %s", key_.c_str(), value_.c_str()));

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

	bool SetXx(const std::wstring& key_, const std::string& value_)
	{
		redisContext* rc = AllocateConnection();
		redisReply* reply = reinterpret_cast<redisReply*>(redisCommand(rc, "SET %s %s XX", key_.c_str(), value_.c_str()));
		
		DeallocateConnection(rc);
		if (reply != nullptr)
		{
			if (reply->type == REDIS_REPLY_STATUS)
			{
				if (strcmp(reply->str, "OK") == 0)
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
			else if (reply->type == REDIS_REPLY_ERROR)
			{
				std::cerr << "RedisManager::SetXx : " << reply->str << "\n";
			}
		}

		freeReplyObject(reply);
		return false;
	}

	// for make session
	bool SetNx(const int key_, const std::wstring& value_)
	{
		redisContext* rc = AllocateConnection();
		redisReply* reply = reinterpret_cast<redisReply*>(redisCommand(rc, "SETNX %d %s", key_, value_.c_str()));

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

	// ������ �������� Ű���� �״�� ������ �ȴ�.
	bool Lock(const std::wstring& lockname_)
	{
		std::wstring key = L"lock" + lockname_;

		redisContext* rc = AllocateConnection();
		redisReply* reply = reinterpret_cast<redisReply*>(redisCommand(rc, "SET %s %s NX PX %d", key.c_str(), "LOCK", REDIS_LOCK_TIME_MILLISECS));
		DeallocateConnection(rc);

		if (reply != nullptr)
		{
			if (reply->type == REDIS_REPLY_STATUS &&
				strcmp(reply->str, "OK") == 0)
			{
				freeReplyObject(reply);
				return true;
			}
		}

		freeReplyObject(reply);
		return false;
	}

	// ������ �������� Ű���� �״�� ������ �ȴ�.
	bool Unlock(const std::wstring& lockname_)
	{
		std::wstring key = L"lock" + lockname_;

		if (Del(key))
		{
			return true;
		}
		else
		{
			return false;
		}
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
	const int REDIS_LOCK_TIME_MILLISECS = 10000;
};