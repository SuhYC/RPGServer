#pragma once

#include "PacketData.hpp"
#include "CharInfo.hpp"
#include "RedisConnectionPool.hpp"
#include "Json.hpp"
#include "Inventory.hpp"

/*
* 레디스에 저장할 키는 다음과 같은 규칙을 따른다.
* 1. int형 숫자
*  - 로그인된 계정의 유저번호를 의미한다. value는 ip정도를 넣을 생각
* 2. int형 숫자 + 인스턴스이름(std::string)
*  - 특정캐릭터(혹은 npc)의 정보가 담긴 구조체를 Json화 하여 value로 저장.
* 3. "lock" + int형 숫자 + 인스턴스이름(std::string)
*  - 2번 데이터를 변경하기 위한 레디스분산락. 사용 후 반드시 해제하여야한다.
*  - 분산락을 걸고 해제하기 전에 서버가 다운되면 큰일이다.
*  - 유효기간을 걸어야겠다. 10초 정도로 할까?
* 4. "ReserveNick" + std::string
*  - 캐릭터 생성에 있어 닉네임을 예약하고 생성여부를 재확인하기 위해 만든 락.
*  - 3번과 마찬가지로 유효기간을 건다.
*
* 레디스 분산락
* 1. 읽기에는 쓰지 않는다.
* 2. 초기 업로드에는 쓰지 않는다. (Nx로 저장하면 된다.)
* 3. 초기 업로드 이후 어떠한 동작에 대한 수정에만 쓴다.
*  ex) BuyItem(const int charno, const int price) -> must use lock
*
* 데이터에 유효기간을 지정하는건 분산락밖에 없다.
* 만약 데이터 저장량이 너무 많아 레디스서버가 문제가 된다면 유효기간 설정을 통해 오랫동안 사용되지 않은 데이터를 자동으로 지우면 된다. (EX : secs, PX : msecs)
*/

enum class REDISRETURN
{
	WRONG_PARAM,	// 잘못된 입력
	LOCK_FAILED,	// 락 획득 실패 (재도전해야함)
	FAIL,			// 기타 이유로 실패
	SUCCESS			// 반영 성공
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

	// Update Info of Char on Redis
	// data must be on redis
	bool UpdateCharInfo(const int charNo_, const std::string& info_)
	{
		std::string key = std::to_string(charNo_) + "CharInfo";
		return SetXx(key, info_);
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

	bool CreateInven(const int charNo_, std::string& strInven_)
	{
		std::string key = std::to_string(charNo_) + "Inven";

		return SetNx(key, strInven_);
	}

	bool GetInven(const int charNo_, std::string& out_)
	{
		std::string key = std::to_string(charNo_) + "Inven";
		return Get(key, out_);
	}

	REDISRETURN SetGold(const int charNo_)
	{
		std::string key = std::to_string(charNo_) + "CharInfo";

		if (!Lock(key))
		{
			return REDISRETURN::LOCK_FAILED;
		}

		std::string strInfo;

		if (!GetCharInfo(charNo_, strInfo))
		{
			std::cerr << "RedisManager::SetGold : CharInfo 검색 실패\n";
			Unlock(key);
			return REDISRETURN::FAIL;
		}

		CharInfo info = { 0 };
		if (!m_jsonMaker.ToCharInfo(strInfo, info))
		{
			std::cerr << "RedisManager::SetGold : CharInfo 역직렬화 실패\n";
			Unlock(key);
			return REDISRETURN::FAIL;
		}

		info.Gold = 10000;

		if (!m_jsonMaker.ToJsonString(info, strInfo))
		{
			std::cerr << "RedisManager::SetGold : CharInfo 직렬화 실패\n";
			Unlock(key);
			return REDISRETURN::FAIL;
		}

		if (!SetXx(key, strInfo))
		{
			std::cerr << "RedisManager::SetGold : SetXx 실패\n";
			Unlock(key);
			return REDISRETURN::FAIL;
		}

		Unlock(key);
		return REDISRETURN::SUCCESS;
	}

	// 아이템가격과 플레이어의 소지금만 확인한다.
	// REDISRETURN::WRONG_PARAM : 잘못된 파라미터기 때문에 재시도하지 않는다.
	// REDISRETURN::LOCK_FAILED : 다른 스레드가 접근중이다. 재시도해도 된다.
	// REDISRETURN::FAIL : 레디스 혹은 RapidJson의 수행 도중 발생한 실패.
	// REDISRETURN::SUCCESS : 구매반영 성공
	REDISRETURN BuyItem(const int charNo_, const int itemcode_, const time_t extime_, const int count_, const int price_)
	{
		// 구매 갯수 제한
		if (count_ > MAX_COUNT_ON_SLOT)
		{
			std::cerr << "RedisManager::BuyItem : Too Much Count\n";
			return REDISRETURN::WRONG_PARAM;
		}

		std::string infokey = std::to_string(charNo_) + "CharInfo";
		std::string invenkey = std::to_string(charNo_) + "Inven";

		// 락 설정 실패
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
			std::cerr << "RedisManager::BuyItem : CharInfo검색 실패\n";
			Unlock(infokey);
			Unlock(invenkey);
			return REDISRETURN::FAIL;
		}
		std::string strInven;
		if (!GetInven(charNo_, strInven))
		{
			std::cerr << "RedisManager::BuyItem : Inven검색 실패\n";
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

		// 인벤토리에 추가
		if (!inven.push_back(itemcode_, extime_, count_))
		{
			std::cout << "RedisManager::BuyItem : Not Enough Available Space On Inven\n";
			Unlock(infokey);
			Unlock(invenkey);
			return REDISRETURN::FAIL;
		}

		info.Gold -= price_ * count_;

		std::string newStrInfo;

		if (!m_jsonMaker.ToJsonString(info, newStrInfo) ||
			!m_jsonMaker.ToJsonString(inven, strInven))
		{
			std::cerr << "RedisManager::BuyItem : Json문자열 생성 실패\n";
			Unlock(infokey);
			Unlock(invenkey);
			return REDISRETURN::FAIL;
		}

		if (!SetXx(infokey, newStrInfo))
		{
			std::cerr << "RedisManager::BuyItem : Info Set Failed\n";
			Unlock(infokey);
			Unlock(invenkey);
			return REDISRETURN::FAIL;
		}
		// 이 경우는 수정했던 것을 되돌려야한다.
		if (!SetXx(invenkey, strInven))
		{
			std::cerr << "RedisManager::BuyItem : inven Set Failed\n";
			SetXx(infokey, strInfo); // 되돌리기
			Unlock(infokey);
			Unlock(invenkey);
			return REDISRETURN::FAIL;
		}


		Unlock(infokey);
		Unlock(invenkey);
		return REDISRETURN::SUCCESS;
	}

	REDISRETURN AddItem(const int charNo_, const int itemCode_, const time_t exTime_, const int count_)
	{
		// 구매 갯수 제한
		if (count_ > MAX_COUNT_ON_SLOT)
		{
			std::cerr << "RedisManager::AddItem : Too Much Count\n";
			return REDISRETURN::WRONG_PARAM;
		}

		std::string invenkey = std::to_string(charNo_) + "Inven";

		if (!Lock(invenkey))
		{
			return REDISRETURN::LOCK_FAILED;
		}

		std::string strInven;
		if (!GetInven(charNo_, strInven))
		{
			std::cerr << "RedisManager::AddItem : Inven검색 실패\n";
			Unlock(invenkey);
			return REDISRETURN::FAIL;
		}

		Inventory inven;
		m_jsonMaker.ToInventory(strInven, inven);

		// 인벤토리에 추가
		if (!inven.push_back(itemCode_, exTime_, count_))
		{
			std::cout << "RedisManager::AddItem : Not Enough Available Space On Inven\n";
			Unlock(invenkey);
			return REDISRETURN::FAIL;
		}

		std::string newStrInfo;

		if (!m_jsonMaker.ToJsonString(inven, strInven))
		{
			std::cerr << "RedisManager::AddItem : Json문자열 생성 실패\n";
			Unlock(invenkey);
			return REDISRETURN::FAIL;
		}

		if (!SetXx(invenkey, strInven))
		{
			std::cerr << "RedisManager::AddItem : inven Set Failed\n";
			Unlock(invenkey);
			return REDISRETURN::FAIL;
		}

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
			return REDISRETURN::WRONG_PARAM; // Drop은 실패하는 케이스가 잘못된 매개변수밖에 없다.
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

	REDISRETURN SwapInven(const int charCode_, const int idx1, const int idx2)
	{
		if (idx1 == idx2 || idx1 >= MAX_SLOT || idx2 >= MAX_SLOT || idx1 < 0 || idx2 < 0)
		{
			return REDISRETURN::WRONG_PARAM;
		}

		std::string key = std::to_string(charCode_) + "Inven";

		if (!Lock(key))
		{
			return REDISRETURN::LOCK_FAILED;
		}

		std::string strInven;
		if (!GetInven(charCode_, strInven))
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

		stInven.Swap(idx1, idx2);

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

	// 캐릭터 생성을 위해 닉네임을 예약한다.
	// N분 동안 예약할 수 있도록 하자. (lock을 거는거나 마찬가지니..)
	bool ReserveNickname(std::string& nickname_, const int lockTimeMillisec_ = RESERVE_NICKNAME_TIME_MILLISECS)
	{
		std::string key = "ReserveNick" + nickname_;

		redisContext* rc = AllocateConnection();
		redisReply* reply = reinterpret_cast<redisReply*>(redisCommand(rc, "SET %s %s NX PX %d", key.c_str(), "LOCK", lockTimeMillisec_));
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

	// ReserveNickname을 취소한다.
	// 반드시 해당 닉네임을 예약하는데 성공한 유저인지 확인하고 시도할 것.
	bool CancelReserveNickname(std::string& nickname_)
	{
		std::string key = "ReserveNick" + nickname_;

		if (Del(key))
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	bool CreateSalesList(const int npcCode_, std::string& strSalesList)
	{
		std::string key = std::to_string(npcCode_) + "SalesList";

		return SetNx(key, strSalesList);
	}

	bool GetSalesList(const int npcCode_, std::string& out_)
	{
		std::string key = std::to_string(npcCode_) + "SalesList";

		return Get(key, out_);
	}

	REDISRETURN MoveMap(const int charCode_, const int mapCode_)
	{
		std::string key = std::to_string(charCode_) + "CharInfo";

		if (!Lock(key))
		{
			return REDISRETURN::LOCK_FAILED;
		}

		std::string strInfo;

		if (!Get(key, strInfo))
		{
			Unlock(key);
			return REDISRETURN::FAIL;
		}

		CharInfo stInfo = { 0 };
		m_jsonMaker.ToCharInfo(strInfo, stInfo);

		stInfo.LastMapCode = mapCode_;

		if (!m_jsonMaker.ToJsonString(stInfo, strInfo))
		{
			Unlock(key);
			return REDISRETURN::FAIL;
		}

		if (!SetXx(key, strInfo))
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

	// 수정할 데이터의 키값을 그대로 넣으면 된다.
	bool Lock(const std::string& lockname_, const int lockTimeMillisec_ = REDIS_LOCK_TIME_MILLISECS)
	{
		std::string key = "lock" + lockname_;

		redisContext* rc = AllocateConnection();
		redisReply* reply = reinterpret_cast<redisReply*>(redisCommand(rc, "SET %s %s NX PX %d", key.c_str(), "LOCK", lockTimeMillisec_));
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

	static const int REDIS_LOCK_TIME_MILLISECS = 10000;
	static const int RESERVE_NICKNAME_TIME_MILLISECS = 60 * 10 * 1000; // 일단 10분
};