#pragma once

#include "PacketData.hpp"
#include "CharInfo.hpp"
#include "RedisConnectionPool.hpp"
#include "Json.hpp"
#include "Inventory.hpp"

/*
* ���𽺿� ������ Ű�� ������ ���� ��Ģ�� ������.
* 1. int�� ����
*  - �α��ε� ������ ������ȣ�� �ǹ��Ѵ�. value�� ip������ ���� ����
* 2. int�� ���� + �ν��Ͻ��̸�(std::string)
*  - Ư��ĳ������ ������ ��� ����ü�� Jsonȭ �Ͽ� value�� ����.
* 3. "lock" + int�� ���� + �ν��Ͻ��̸�(std::string)
*  - 2�� �����͸� �����ϱ� ���� ���𽺺л��. ��� �� �ݵ�� �����Ͽ����Ѵ�.
*  - �л���� �ɰ� �����ϱ� ���� ������ �ٿ�Ǹ� ū���̴�.
*  - ��ȿ�Ⱓ�� �ɾ�߰ڴ�. 10�� ������ �ұ�?
*
* ���� �л��
* 1. �б⿡�� ���� �ʴ´�.
* 2. �ʱ� ���ε忡�� ���� �ʴ´�. (Nx�� �����ϸ� �ȴ�.)
* 3. �ʱ� ���ε� ���� ��� ���ۿ� ���� �������� ����.
*  ex) BuyItem(const int charno, const int price) -> must use lock
*
* �����Ϳ� ��ȿ�Ⱓ�� �����ϴ°� �л���ۿ� ����.
* ���� ������ ���差�� �ʹ� ���� ���𽺼����� ������ �ȴٸ� ��ȿ�Ⱓ ������ ���� �������� ������ ���� �����͸� �ڵ����� ����� �ȴ�. (EX : secs, PX : msecs)
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
	bool MakeSession(const int usercode_, const std::string& ip_)
	{
		return SetNx(usercode_, ip_);
	}

	// Create Info of Char on Redis(Cache)
	// data must not be on redis
	bool CreateCharInfo(const int charNo_, const std::string& info_)
	{
		std::string key = std::to_string(charNo_) + "CharInfo";

		return SetNx(key, info_);
	}

	bool GetCharInfo(const int charNo_, std::string& out_)
	{
		std::string key = std::to_string(charNo_) + "CharInfo";
		return Get(key, out_);
	}

	void LogOut(const int userCode_)
	{
		Del(userCode_);

		return;
	}

	bool GetInven(const int charNo_, std::string& out_)
	{
		std::string key = std::to_string(charNo_) + "Inven";
		return Get(key, out_);
	}

	bool BuyItem(const int charNo_, const int itemcode_, const time_t extime_, const int count_, const int price_)
	{
		// ���� ���� ����
		if (count_ > 100)
		{
			return false;
		}

		std::string infokey = std::to_string(charNo_) + "CharInfo";
		std::string invenkey = std::to_string(charNo_) + "Inven";

		// �� ���� ����
		if (!Lock(infokey))
		{
			return false;
		}
		if (!Lock(invenkey))
		{
			Unlock(infokey);
			return false;
		}

		std::string strInfo;
		if (!GetCharInfo(charNo_, strInfo))
		{
			std::cerr << "RedisManager::BuyItem : CharInfo�˻� ����\n";
			Unlock(infokey);
			Unlock(invenkey);
			return false;
		}
		std::string strInven;
		if (!GetInven(charNo_, strInven))
		{
			std::cerr << "RedisManager::BuyItem : Inven�˻� ����\n";
			Unlock(infokey);
			Unlock(invenkey);
			return false;
		}

		CharInfo info;
		m_jsonMaker.ToCharInfo(strInfo, info);

		if (info.Gold < price_ * count_)
		{
			Unlock(infokey);
			Unlock(invenkey);
			return false;
		}

		Inventory inven;
		//m_jsonMaker.ToInventory(strinven, inven);

		// �κ��丮�� �߰�
		if (!inven.push_back(itemcode_, extime_, count_))
		{
			Unlock(infokey);
			Unlock(invenkey);
			return false;
		}

		info.Gold -= price_ * count_;

		std::string newStrInfo;

		if (!m_jsonMaker.ToJsonString(info, newStrInfo)) //||
			//!m_jsonMaker.ToJsonString(&inven, strinven))
		{
			std::cerr << "RedisManager::BuyItem : Json���ڿ� ���� ����\n";
			Unlock(infokey);
			Unlock(invenkey);
			return false;
		}

		if (!SetXx(infokey, newStrInfo))
		{
			Unlock(infokey);
			Unlock(invenkey);
			return false;
		}
		// �� ���� �����ߴ� ���� �ǵ������Ѵ�.
		if (!SetXx(invenkey, strInven))
		{
			SetXx(infokey, strInfo); // �ǵ�����
			Unlock(infokey);
			Unlock(invenkey);
			return false;
		}


		Unlock(infokey);
		Unlock(invenkey);
		return true;
	}

private:
	// for create data
	// str(usercode+datatype) : key_
	// jsonstr : value_
	bool SetNx(const std::string& key_, const std::string& value_)
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

	bool SetXx(const std::string& key_, const std::string& value_)
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
	bool SetNx(const int key_, const std::string& value_)
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

	bool Del(const std::string& key_)
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

	bool Get(const std::string& key_, std::string& out_)
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
	bool Lock(const std::string& lockname_)
	{
		std::string key = "lock" + lockname_;

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
	bool Unlock(const std::string& lockname_)
	{
		std::string key = "lock" + lockname_;

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