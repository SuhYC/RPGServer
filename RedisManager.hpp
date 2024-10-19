#pragma once

#include "PacketData.hpp"
#include "CharInfo.hpp"
#include "RedisConnectionPool.hpp"
#include "Json.hpp"

/*
* 레디스에 저장할 키는 다음과 같은 규칙을 따른다.
* 1. int형 숫자
*  - 로그인된 계정의 유저번호를 의미한다. value는 ip정도를 넣을 생각
* 2. int형 숫자 + 인스턴스이름(std::wstring)
*  - 특정캐릭터의 정보가 담긴 구조체를 Json화 하여 value로 저장.
* 3. L"lock" + int형 숫자 + 인스턴스이름(std::wstring)
*  - 2번 데이터를 변경하기 위한 레디스분산락. 사용 후 반드시 해제하여야한다.
*  - 분산락을 걸고 해제하기 전에 서버가 다운되면 큰일이다.
*  - 유효기간을 걸어야겠다. 10초 정도로 할까?
* 
* 레디스 분산락
* 1. 읽기에는 쓰지 않는다.
* 2. 초기 업로드에는 쓰지 않는다.
* 3. 초기 업로드 이후 어떠한 동작에 대한 수정에만 쓴다.
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

	// 수정할 데이터의 키값을 그대로 넣으면 된다.
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

	// 수정할 데이터의 키값을 그대로 넣으면 된다.
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