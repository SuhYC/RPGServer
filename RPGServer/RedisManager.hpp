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

enum class REDISRETURN
{
	WRONG_PARAM,	// �߸��� �Է�
	LOCK_FAILED,	// �� ȹ�� ���� (�絵���ؾ���)
	FAIL,			// ��Ÿ ������ ����
	SUCCESS			// �ݿ� ����
};

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

	// �����۰��ݰ� �÷��̾��� �����ݸ� Ȯ���Ѵ�.
	// REDISRETURN::WRONG_PARAM : �߸��� �Ķ���ͱ� ������ ��õ����� �ʴ´�.
	// REDISRETURN::LOCK_FAILED : �ٸ� �����尡 �������̴�. ��õ��ص� �ȴ�.
	// REDISRETURN::FAIL : ���� Ȥ�� RapidJson�� ���� ���� �߻��� ����.
	// REDISRETURN::SUCCESS : ���Źݿ� ����
	REDISRETURN BuyItem(const int charNo_, const int itemcode_, const time_t extime_, const int count_, const int price_)
	{
		// ���� ���� ����
		if (count_ > MAX_COUNT_ON_SLOT)
		{
			return REDISRETURN::WRONG_PARAM;
		}

		std::string infokey = std::to_string(charNo_) + "CharInfo";
		std::string invenkey = std::to_string(charNo_) + "Inven";

		// �� ���� ����
		if (!Lock(infokey))
		{
			return REDISRETURN::LOCK_FAILED;
		}
		if (!Lock(invenkey))
		{
			Unlock(infokey);
			return REDISRETURN::LOCK_FAILED;
		}

		std::string strInfo;
		if (!GetCharInfo(charNo_, strInfo))
		{
			std::cerr << "RedisManager::BuyItem : CharInfo�˻� ����\n";
			Unlock(infokey);
			Unlock(invenkey);
			return REDISRETURN::FAIL;
		}
		std::string strInven;
		if (!GetInven(charNo_, strInven))
		{
			std::cerr << "RedisManager::BuyItem : Inven�˻� ����\n";
			Unlock(infokey);
			Unlock(invenkey);
			return REDISRETURN::FAIL;
		}

		CharInfo info;
		m_jsonMaker.ToCharInfo(strInfo, info);

		if (info.Gold < price_ * count_)
		{
			Unlock(infokey);
			Unlock(invenkey);
			return REDISRETURN::FAIL;
		}

		Inventory inven;
		m_jsonMaker.ToInventory(strInven, inven);

		// �κ��丮�� �߰�
		if (!inven.push_back(itemcode_, extime_, count_))
		{
			Unlock(infokey);
			Unlock(invenkey);
			return REDISRETURN::FAIL;
		}

		info.Gold -= price_ * count_;

		std::string newStrInfo;

		if (!m_jsonMaker.ToJsonString(info, newStrInfo) ||
			!m_jsonMaker.ToJsonString(inven, strInven))
		{
			std::cerr << "RedisManager::BuyItem : Json���ڿ� ���� ����\n";
			Unlock(infokey);
			Unlock(invenkey);
			return REDISRETURN::FAIL;
		}

		if (!SetXx(infokey, newStrInfo))
		{
			Unlock(infokey);
			Unlock(invenkey);
			return REDISRETURN::FAIL;
		}
		// �� ���� �����ߴ� ���� �ǵ������Ѵ�.
		if (!SetXx(invenkey, strInven))
		{
			SetXx(infokey, strInfo); // �ǵ�����
			Unlock(infokey);
			Unlock(invenkey);
			return REDISRETURN::FAIL;
		}


		Unlock(infokey);
		Unlock(invenkey);
		return REDISRETURN::SUCCESS;
	}

	REDISRETURN DropItem(const int charNo_, const int slotIdx_, const int count_, Item& out_)
	{
		if (count_ > MAX_COUNT_ON_SLOT || count_ <= 0 || slotIdx_ >= MAX_SLOT || slotIdx_ < 0)
		{
			return REDISRETURN::WRONG_PARAM;
		}

		std::string key = std::to_string(charNo_) + "Inven";

		if (!Lock(key))
		{
			return REDISRETURN::LOCK_FAILED;
		}

		std::string strInven;

		if (!GetInven(charNo_, strInven))
		{
			Unlock(key);
			return REDISRETURN::FAIL;
		}

		Inventory stInven;

		if (!m_jsonMaker.ToInventory(strInven, stInven))
		{
			Unlock(key);
			return REDISRETURN::FAIL;
		}

		out_.Init(stInven[slotIdx_]);

		if (!stInven.Drop(slotIdx_, count_))
		{
			Unlock(key);
			return REDISRETURN::WRONG_PARAM; // Drop�� �����ϴ� ���̽��� �߸��� �Ű������ۿ� ����.
		}

		if (!m_jsonMaker.ToJsonString(stInven, strInven))
		{
			Unlock(key);
			return REDISRETURN::FAIL;
		}

		if (!SetXx(key, strInven))
		{
			Unlock(key);
			return REDISRETURN::FAIL;
		}

		Unlock(key);
		return REDISRETURN::SUCCESS;
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