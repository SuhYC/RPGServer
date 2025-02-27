#pragma once

/*
* 해당 파일에서 MSSQL, Redis와 관련된 기능을 관리한다.
* Redis관련 기능은 RedisManager에서 hiredis관련 기능을 사용한다.
* 
* 구문핸들을 여러개 사용한다 하더라도 하나의 hdbc에 연결하는건 thread-safe하지 않을 수 있다는 의견.
* -> 각각의 구문핸들마다 환경,연결핸들을 따로 설정하자.
* 
* signin -> userno 획득
* GetCharList(userno) -> charno + ... 획득 TABLE(CHARLIST)
* GetCharInfo(charno) -> 
* 
* CharList, CharInfo 인스턴스는 사용 후 여기로 반납할 수 있게 함수를 만들자.
*/

#include "ODBCConnectionPool.hpp"
#include "RedisManager.hpp"
#include "CharList.hpp"
#include "CharInfo.hpp"
#include "MemoryPool.hpp"
#include <unordered_map>
#include "UMapWrapper.hpp"
#include "MonsterInfo.hpp"
#include "MonsterDropInfo.hpp"
#include <time.h>

const bool DB_DEBUG = true;
const int CLIENT_NOT_CERTIFIED = 0;
const int ALREADY_HAVE_SESSION = -1;
const int CHARINFO_MEMORY_POOL_COUNT = 100;
const int CHARLIST_MEMORY_POOL_COUNT = 100;

enum class eReturnCode
{
	SIGNUP_SUCCESS,
	SIGNUP_ALREADY_IN_USE,
	SIGNUP_FAIL,
	SYSTEM_ERROR // 시스템 문제
};

class Database
{
public:
	Database()
	{
		m_Pool = std::make_unique<MSSQL::ConnectionPool>();
		m_CharInfoPool = std::make_unique<MemoryPool<CharInfo>>(CHARINFO_MEMORY_POOL_COUNT);
		m_CharListPool = std::make_unique<MemoryPool<CharList>>(CHARLIST_MEMORY_POOL_COUNT);
		MakeReservedWordList();

		MakePriceTable();
		MakeSalesList();
		MakeMapEdgeList();
		MakeMapNPCInfo();
	}

	virtual ~Database()
	{
		m_Pool.reset();
		m_CharInfoPool.reset();
		m_CharListPool.reset();
	}

	// eReturnCode : SIGNUP_FAIL, SIGNUP_SUCCESS, SIGNUP_ALREADY_IN_USE
	eReturnCode SignUp(const std::string& id_, const std::string& pw_,
		const int questno_, const std::string& ans_, const std::string& hint_)
	{
		if (!CheckPWIsValid(pw_) || !CheckIDIsValid(id_) ||
			!InjectionCheck(ans_) || !InjectionCheck(hint_))
		{
			// 유효하지 않은 입력
			return eReturnCode::SIGNUP_FAIL;
		}

		HANDLE hstmt = m_Pool->Allocate();

		if (hstmt == INVALID_HANDLE_VALUE)
		{
			std::cerr << "Database::SignUp : 구문핸들 발급실패\n";
			return eReturnCode::SYSTEM_ERROR;
		}

		std::wstring wid = std::wstring(id_.begin(), id_.end());
		std::wstring wpw = std::wstring(pw_.begin(), pw_.end());
		std::wstring whint = std::wstring(hint_.begin(), hint_.end());
		std::wstring wans = std::wstring(ans_.begin(), ans_.end());

		SQLWCHAR query[512] = { 0 };
		swprintf((wchar_t*)query, 512,
			L"IF NOT EXISTS (SELECT 1 FROM LOGINDATA WHERE USERID = N'%s') "
			L"BEGIN "
			L"    INSERT INTO LOGINDATA (USERID, PASSWORD, QUESTNO, HINT, ANSWER) VALUES (N'%s', N'%s', %d, N'%s', N'%s'); "
			L"END ", wid.c_str(), wid.c_str(), wpw.c_str(), questno_, whint.c_str(), wans.c_str());

		SQLRETURN retCode = SQLExecDirect(hstmt, query, SQL_NTS);

		// SQLExecute Failed
		if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO)
		{
			ReleaseHandle(hstmt);
			std::cerr << "Database::SIGNUP : Failed to Execute\n";
			return eReturnCode::SYSTEM_ERROR;
		}

		SQLLEN rowCount = 0;
		SQLRowCount(hstmt, &rowCount);

		if (rowCount == 1)
		{
			return eReturnCode::SIGNUP_SUCCESS;
		}
		else
		{
			return eReturnCode::SIGNUP_ALREADY_IN_USE;
		}
	}

	// usercode:int로 리턴.
	// 0 : Client Not Certified.
	// -1 : Already Have Session
	int SignIn(const std::string& id_, const std::string& pw_, const std::string& ip_)
	{
		if (!CheckIDIsValid(id_) || !CheckPWIsValid(pw_))
		{
			return CLIENT_NOT_CERTIFIED;
		}

		HANDLE hstmt = m_Pool->Allocate();
		
		if (hstmt == INVALID_HANDLE_VALUE)
		{
			std::cerr << "Database::SignIn : 구문핸들 발급 실패\n";
			return CLIENT_NOT_CERTIFIED;
		}

		std::wstring wid = std::wstring(id_.begin(), id_.end());
		std::wstring wpw = std::wstring(pw_.begin(), pw_.end());

		SQLWCHAR query[256] = { 0 };
		swprintf((wchar_t*)query, 256,
			L"SELECT USERNO FROM LOGINDATA WHERE USERID = N'%s' AND PASSWORD = N'%s' "
			, wid.c_str(), wpw.c_str());

		if (DB_DEBUG)
		{
			//std::cout << "Database::SignIn : id : " << id_ << ", pw : " << pw_ << '\n';
			//std::wcout << "Database::SignIn : query : " << query << '\n';
		}

		SQLRETURN retCode = SQLExecDirect(hstmt, query, SQL_NTS);

		// SQLExecute Failed
		if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO)
		{
			std::wcerr << L"DB::SIGNIN : Failed to Execute\n";

			ReleaseHandle(hstmt);
			return CLIENT_NOT_CERTIFIED;
		}

		SQLINTEGER result = 0;
		
		retCode = SQLBindCol(hstmt, 1, SQL_C_SLONG, &result, 0, nullptr);

		if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO)
		{
			std::wcerr << L"DB::SIGNIN : Failed to Bind Column\n";

			ReleaseHandle(hstmt);
			return CLIENT_NOT_CERTIFIED;
		}

		retCode = SQLFetch(hstmt);

		// CLIENT_NOT_CERTIFIED : WRONG ID OR PW
		if (retCode == SQL_NO_DATA)
		{
			std::wcerr << L"DB::SIGNIN : NODATA\n";
			ReleaseHandle(hstmt);
			return CLIENT_NOT_CERTIFIED;
		}

		// SQLFetch Failed
		if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO)
		{
			std::wcerr << L"DB::SIGNIN : Failed to Fetch\n";
			ReleaseHandle(hstmt);
			return CLIENT_NOT_CERTIFIED;
		}

		if (DB_DEBUG)
		{
			std::cout << "SIGNIN RESULT : " << result << "\n";
		}

		ReleaseHandle(hstmt);
		if (TrySignIn(result, ip_))
		{
			return result;
		}
		else
		{
			return ALREADY_HAVE_SESSION; // other user using this id.
		}

	}

	void LogOut(const int userCode_)
	{
		m_RedisManager.LogOut(userCode_);
	}
	
	// CharInfo Class 만들어야함 + 메모리풀도 -> 패킷데이터랑 템플릿으로 묶을까?
	// Redis 먼저 조회하고 없으면 MSSQL 조회
	// 해당 No에 맞는 캐릭터의 정보를 담아 온다.
	std::string GetCharInfoJsonStr(const int charNo_)
	{
		std::string ret;
		// CacheServer 먼저 확인
		if (m_RedisManager.GetCharInfo(charNo_, ret))
		{
			return ret;
		}

		HANDLE hstmt = m_Pool->Allocate();

		SQLWCHAR query[512] = { 0 };
		swprintf((wchar_t*)query, 512,
			L"SELECT CHARNAME, CLASS_CODE, LV, EXPERIENCE, STAT_POINT, HEALTH_POINT, MANA_POINT, CURRENT_HEALTH, CURRENT_MANA, STRENGTH, DEXTERITY, INTELLIGENCE, MENTALITY, GOLD, LASTMAPCODE "
			L"FROM CHARINFO "
			L"WHERE CHARNO = %d; "
			, charNo_);

		if (DB_DEBUG)
		{
			//std::cout << "Database::GetCharInfoJsonStr : No. : " << charNo_ << '\n';
			//std::wcout << "Database::GetCharInfoJsonStr : query : " << query << '\n';
		}

		SQLRETURN retCode = SQLExecDirect(hstmt, query, SQL_NTS);

		// SQLExecute Failed
		if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO)
		{
			PrintError(hstmt);
			std::wcerr << L"Database::GetCharInfoJsonStr : Failed to Execute\n";
			ReleaseHandle(hstmt);
			return std::string();
		}

		retCode = SQLFetch(hstmt);

		// SQLFetch Failed
		if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO)
		{
			std::wcerr << L"Database::GetCharInfoJsonStr : Failed to Fetch\n";
			ReleaseHandle(hstmt);
			return std::string();
		}

		CharInfo* pCharInfo = m_CharInfoPool->Allocate();

		if (pCharInfo == nullptr)
		{
			std::cerr << "Database::GetCharInfoJsonStr : Failed to Allocate Info Instance\n";
			ReleaseHandle(hstmt);
			return std::string();
		}

		SQLLEN len;
		SQLWCHAR name[20] = { 0 };

		SQLGetData(hstmt, 1, SQL_C_WCHAR, name, sizeof(name) / sizeof(name[0]), &len);
		SQLGetData(hstmt, 2, SQL_C_LONG, &(pCharInfo->ClassCode), 0, &len);
		SQLGetData(hstmt, 3, SQL_C_LONG, &(pCharInfo->Level), 0, &len);
		SQLGetData(hstmt, 4, SQL_C_LONG, &(pCharInfo->Experience), 0, &len);
		SQLGetData(hstmt, 5, SQL_C_LONG, &(pCharInfo->StatPoint), 0, &len);
		SQLGetData(hstmt, 6, SQL_C_LONG, &(pCharInfo->HealthPoint), 0, &len);
		SQLGetData(hstmt, 7, SQL_C_LONG, &(pCharInfo->ManaPoint), 0, &len);
		SQLGetData(hstmt, 8, SQL_C_LONG, &(pCharInfo->CurrentHealth), 0, &len);
		SQLGetData(hstmt, 9, SQL_C_LONG, &(pCharInfo->CurrentMana), 0, &len);
		SQLGetData(hstmt, 10, SQL_C_LONG, &(pCharInfo->Strength), 0, &len);
		SQLGetData(hstmt, 11, SQL_C_LONG, &(pCharInfo->Dexterity), 0, &len);
		SQLGetData(hstmt, 12, SQL_C_LONG, &(pCharInfo->Intelligence), 0, &len);
		SQLGetData(hstmt, 13, SQL_C_LONG, &(pCharInfo->Mentality), 0, &len);
		SQLGetData(hstmt, 14, SQL_C_LONG, &(pCharInfo->Gold), 0, &len);
		SQLGetData(hstmt, 15, SQL_C_LONG, &(pCharInfo->LastMapCode), 0, &len);

		std::wstring wstr(name);
		
		std::string euckrStr = convertUtf16ToEucKr(wstr);
		
		strcpy_s(pCharInfo->CharName, euckrStr.c_str());

		ReleaseHandle(hstmt);

		pCharInfo->CharNo = charNo_;

		m_JsonMaker.ToJsonString(*pCharInfo, ret);

		ReleaseObject(pCharInfo);

		// 레디스에 등록하기
		if (m_RedisManager.CreateCharInfo(charNo_, ret) == false)
		{
			std::wcerr << L"Database::GetCharInfoJsonStr : 레디스에 정보 등록 실패\n";
		}

		return ret;
	}

	// CharInfo Class 만들어야함 + 메모리풀도 -> 패킷데이터랑 템플릿으로 묶을까?
	// Redis 먼저 조회하고 없으면 MSSQL 조회
	// 해당 No에 맞는 캐릭터의 정보를 담아 온다.
	// 반드시 return으로 받아간 CharInfo 객체를 ReleaseObject로 반환해야한다.
	CharInfo* GetCharInfo(const int charNo_)
	{
		CharInfo* ret = m_CharInfoPool->Allocate();

		if (ret == nullptr)
		{
			std::cerr << "DB::GetCharInfo : Failed to Allocate Info Instance\n";
			return nullptr;
		}

		std::string strCharInfo;

		// CacheServer 먼저 확인
		if (m_RedisManager.GetCharInfo(charNo_, strCharInfo))
		{
			m_JsonMaker.ToCharInfo(strCharInfo, *ret);
			return ret;
		}

		HANDLE hstmt = m_Pool->Allocate();

		SQLWCHAR query[512] = { 0 };
		swprintf((wchar_t*)query, 512,
			L"SELECT CHARNAME, CLASS_CODE, LV, EXPERIENCE, STAT_POINT, HEALTH_POINT, MANA_POINT, CURRENT_HEALTH, CURRENT_MANA, STRENGTH, DEXTERITY, INTELLIGENCE, MENTALITY, GOLD, LASTMAPCODE "
			L"FROM CHARINFO "
			L"WHERE CHARNO = %d; "
			, charNo_);

		if (DB_DEBUG)
		{
			//std::cout << "Database::GetCharInfo : No. : " << charNo_ << '\n';
			//std::wcout << "Database::GetCharInfo : query : " << query << '\n';
		}

		SQLRETURN retCode = SQLExecDirect(hstmt, query, SQL_NTS);

		// SQLExecute Failed
		if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO)
		{
			std::wcerr << L"Database::GetCharInfo : Failed to Execute\n";
			ReleaseHandle(hstmt);
			return nullptr;
		}

		retCode = SQLFetch(hstmt);

		// SQLFetch Failed
		if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO)
		{
			std::wcerr << L"Database::GetCharInfo : Failed to Fetch\n";
			ReleaseHandle(hstmt);
			return nullptr;
		}

		SQLLEN len;
		
		SQLWCHAR name[20] = { 0 };

		SQLGetData(hstmt, 1, SQL_C_WCHAR, name, sizeof(name) / sizeof(name[0]), &len);

		SQLGetData(hstmt, 2, SQL_C_LONG, &(ret->ClassCode), 0, &len);
		SQLGetData(hstmt, 3, SQL_C_LONG, &(ret->Level), 0, &len);
		SQLGetData(hstmt, 4, SQL_C_LONG, &(ret->Experience), 0, &len);
		SQLGetData(hstmt, 5, SQL_C_LONG, &(ret->StatPoint), 0, &len);
		SQLGetData(hstmt, 6, SQL_C_LONG, &(ret->HealthPoint), 0, &len);
		SQLGetData(hstmt, 7, SQL_C_LONG, &(ret->ManaPoint), 0, &len);
		SQLGetData(hstmt, 8, SQL_C_LONG, &(ret->CurrentHealth), 0, &len);
		SQLGetData(hstmt, 9, SQL_C_LONG, &(ret->CurrentMana), 0, &len);
		SQLGetData(hstmt, 10, SQL_C_LONG, &(ret->Strength), 0, &len);
		SQLGetData(hstmt, 11, SQL_C_LONG, &(ret->Dexterity), 0, &len);
		SQLGetData(hstmt, 12, SQL_C_LONG, &(ret->Intelligence), 0, &len);
		SQLGetData(hstmt, 13, SQL_C_LONG, &(ret->Mentality), 0, &len);
		SQLGetData(hstmt, 14, SQL_C_LONG, &(ret->Gold), 0, &len);
		SQLGetData(hstmt, 15, SQL_C_LONG, &(ret->LastMapCode), 0, &len);

		std::wstring wstr(name);

		std::string euckrStr = convertUtf16ToEucKr(wstr);

		strcpy_s(ret->CharName, euckrStr.c_str());

		ReleaseHandle(hstmt);

		ret->CharNo = charNo_;

		if (!m_JsonMaker.ToJsonString(*ret, strCharInfo))
		{
			std::cerr << "Database::GetCharInfo : Json직렬화 실패.\n";
			return ret;
		}

		// 레디스에 등록하기
		if (m_RedisManager.CreateCharInfo(charNo_, strCharInfo) == false)
		{
			std::wcerr << L"Database::GetCharInfo : 레디스에 정보 등록 실패\n";
		}

		return ret;
	}

	// ToJsonException이 발생할 수 있음.
	// CharList Class 만들어야함 + 메모리풀도 -> 패킷데이터랑 템플릿으로 묶을까?
	// CharNo 여러개를 담아 온다.
	// CharList는 Redis에 담지 않는다. (유저 요청이 접속당 1번수준. 추후 적절한 유저가 캐릭터선택을 요청하는지는 확인하게 만들어야할까?)
	std::string GetCharList(const int userCode_)
	{
		HANDLE hstmt = m_Pool->Allocate();
		if (hstmt == INVALID_HANDLE_VALUE)
		{
			std::cerr << "DB::GetCharList : Invalid handle\n";
			return "";
		}

		SQLWCHAR query[256] = { 0 };
		swprintf((wchar_t*)query, 256,
			L"SELECT CHARNO FROM CHARLIST WHERE USERNO = %d; "
			, userCode_);

		if (DB_DEBUG)
		{
			//std::cout << "Database::GetCharList : usercode : " << userCode_ << '\n';
			//std::wcout << "Database::GetCharList : query : " << query << '\n';
		}

		SQLRETURN retCode = SQLExecDirect(hstmt, query, SQL_NTS);

		// SQLExecute Failed
		if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO)
		{
			std::wcerr << L"Database::GetCharList : Failed to Execute\n";
			ReleaseHandle(hstmt);
			return nullptr;
		}

		std::vector<int> charCodes;
		charCodes.clear();
		int charCode = 0;
		SQLLEN len = 0;

		while (true)
		{
			retCode = SQLFetch(hstmt);
			// End of Result
			if (retCode == SQL_NO_DATA)
			{
				break;
			}

			// SQLFetch Failed
			if (retCode == SQL_ERROR)
			{
				std::wcerr << L"Database::GetCharList : Failed to Fetch\n";
				ReleaseHandle(hstmt);
				return nullptr;
			}

			SQLGetData(hstmt, 1, SQL_C_LONG, &charCode, 0, &len);
			if (DB_DEBUG)
			{
				std::cout << "DB::GetCharList : charcode - {" << charCode << "}\n";
			}
			charCodes.push_back(charCode);
		}

		CharList* pCharList = m_CharListPool->Allocate();

		pCharList->Init(charCodes.size());
		for (size_t i = 0; i < charCodes.size(); i++)
		{
			try {
				(*pCharList)[i] = charCodes[i];
			}
			catch (std::out_of_range e)
			{
				std::wcerr << e.what() << "index : " << i << '\n';
			}
		}

		ReleaseHandle(hstmt);

		std::string ret;
		try
		{
			ret = m_JsonMaker.ToJsonString(*pCharList);
		}
		catch(ToJsonException e)
		{
			ReleaseObject(pCharList);
			throw;
		}

		ReleaseObject(pCharList);
		return ret;
	}

	void ReleaseObject(CharInfo* const pObj_)
	{
		m_CharInfoPool->Deallocate(pObj_);

		return;
	}

	void ReleaseObject(CharList* const pObj_)
	{
		m_CharListPool->Deallocate(pObj_);

		return;
	}

	// 구매가격과 유저의 소지금만 판단한다.
	// 유저의 위치, 권한등은 조사하지 않는다.
	bool BuyItem(const int charCode_, const int itemCode_, const time_t extime_, const int count_)
	{
		int price = GetPrice(itemCode_);

		// 해당하는 아이템 정보 없음
		if (price == -1)
		{
			return false;
		}

		REDISRETURN eRet;

		// 락 획득 실패 시 재도전
		while ((eRet = m_RedisManager.BuyItem(charCode_, itemCode_, extime_, count_, price)) == REDISRETURN::LOCK_FAILED)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(0));
		}

		if (eRet != REDISRETURN::SUCCESS)
		{
			return false;
		}

		return true;
	}

	// return -1 : 해당하는 아이템 정보 없음.
	int GetPrice(const int itemcode_) noexcept
	{
		return m_PriceTable.Get(itemcode_);
	}

	bool FindSalesList(const int npcCode_, const int itemCode_)
	{
		return m_SalesList.Find(npcCode_, itemCode_);
	}

	bool FindMapEdge(const int fromMapCode_, const int toMapCode_)
	{
		return m_MapEdge.Find(fromMapCode_, toMapCode_);
	}

	bool FindNPCInfo(const int mapCode_, const int NPCCode_)
	{
		return m_MapNPCInfo.Find(mapCode_, NPCCode_);
	}

	// 유저가 인벤토리의 아이템을 뿌리는 경우.
	// 뿌려지는 아이템의 코드만 전달한다.
	// return 0 : 실패함, return n : 코드가 n인 아이템을 드랍하는데 성공했다.
	int DropItem(const int charCode_, const int slotIdx_, const int count_)
	{
		REDISRETURN eRet;

		Item stItem;

		while ((eRet = m_RedisManager.DropItem(charCode_, slotIdx_, count_, stItem)) == REDISRETURN::LOCK_FAILED)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(0));
		}

		if (eRet != REDISRETURN::SUCCESS)
		{
			return 0;
		}

		return stItem.itemcode;
	}

	// 몬스터코드에 맞는 드랍정보를 모두 가져온다.
	std::vector<MonsterDropInfo> GetMonsterDropInfo(const int monsterCode_)
	{
		std::vector<MonsterDropInfo> ret;

		auto range = m_DropInfo.Get(monsterCode_);

		for (auto& itr = range.first; itr != range.second; itr++)
		{
			ret.push_back(MonsterDropInfo(itr->second));
		}

		return ret;
	}

	// 지연쓰기를 써보고 싶긴 한데 지연쓰기는 별도의 서버를 하나 구성해야겠다.. (Paging Dirty bit느낌으로 하면 될듯?)
	// 일단 바로바로 반영하는 것으로 작성.
	// 연결을 끊을 때마다 기록하는것도 고려해볼수 있을지도..? (서버가 죽으면 어떡하지..?)
	bool UpdateCharInfo(CharInfo* pInfo_)
	{
		if (pInfo_ == nullptr)
		{
			return false;
		}

		std::string strInfo;

		if (pInfo_->HealthPoint == 0)
		{
			pInfo_->HealthPoint = 100;
		}

		m_JsonMaker.ToJsonString(*pInfo_, strInfo);

		m_RedisManager.UpdateCharInfo(pInfo_->CharNo, strInfo);

		HANDLE hstmt = m_Pool->Allocate();

		SQLWCHAR query[1024] = { 0 };
		swprintf((wchar_t*)query, 1024,
			L"UPDATE CHARINFO SET "
			L"CLASS_CODE = %d, "
			L"LV = %d, EXPERIENCE = %d, STAT_POINT = %d, HEALTH_POINT = %d, MANA_POINT = %d, "
			L"CURRENT_HEALTH = %d, CURRENT_MANA = %d, "
			L"STRENGTH = %d, DEXTERITY = %d, INTELLIGENCE = %d, MENTALITY = %d, GOLD = %d, "
			L"LASTMAPCODE = %d "
			L"WHERE CHARNO = %d;",
			pInfo_->ClassCode, pInfo_->Level, pInfo_->Experience, pInfo_->StatPoint, 
			pInfo_->HealthPoint, pInfo_->ManaPoint, pInfo_->CurrentHealth, pInfo_->CurrentMana,
			pInfo_->Strength, pInfo_->Dexterity, pInfo_->Intelligence, pInfo_->Mentality,
			pInfo_->Gold, pInfo_->LastMapCode,
			pInfo_->CharNo);

		if (DB_DEBUG)
		{
			std::wcout << "Database::UpdateCharInfo : query : " << query << '\n';
		}

		SQLRETURN retCode = SQLExecDirect(hstmt, query, SQL_NTS);

		// SQLExecute Failed
		if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO)
		{
			PrintError(hstmt);
			std::cerr << "Database::UpdateCharInfo : Failed to Execute\n";
			ReleaseHandle(hstmt);
			return false;
		}

		ReleaseHandle(hstmt);

		return true;
	}

	// GameMap의 초기화에만 사용되므로 DB를 바로 조회해도 될것같다.
	// 내부에서 몬스터 드랍정보도 한번에 가져오면 좋을듯
	std::vector<MonsterSpawnInfo> GetMonsterSpawnInfo(const int mapcode_)
	{
		// T(SPAWNINFO) [MAPCODE] [MONSTERCODE] [POSX] [POSY]
		// T(MONSTERINFO)[MONSTERCODE][MAXHEALTH][EXPERIENCE][DROPGOLD]
		// T(DROPINFO) [MONSTERCODE] [ITEMCODE] [RATIO]

		HANDLE hstmt = m_Pool->Allocate();

		if (hstmt == INVALID_HANDLE_VALUE)
		{
			std::cerr << "DB::GetMonsterSpawnInfo : 핸들 발급 실패.\n";
			return std::vector<MonsterSpawnInfo>();
		}

		SQLPrepare(hstmt, (SQLWCHAR*)
			L"SELECT S.MONSTERCODE, S.POSX, S.POSY, M.MAXHEALTH "
			L"FROM " 
			L"	(SELECT MONSTERCODE, POSX, POSY "
			L"	FROM SPAWNINFO "
			L"	WHERE MAPCODE = ?) AS S "
			L"INNER JOIN MONSTERINFO AS M ON S.MONSTERCODE = M.MONSTERCODE ", SQL_NTS);

		int param = mapcode_;
		SQLLEN paramlen;

		SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &param, 0, &paramlen);

		SQLRETURN retCode = SQLExecute(hstmt);

		// SQLExecute Failed
		if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO)
		{
			ReleaseHandle(hstmt);
			std::cerr << "Database::GetMonsterSpawnInfo : Failed to Execute\n";
			return std::vector<MonsterSpawnInfo>();
		}

		std::vector<MonsterSpawnInfo> ret;

		int monstercode;
		Vector2 pos;
		int maxhealth;

		SQLLEN mcodelen;
		SQLLEN posxlen;
		SQLLEN posylen;
		SQLLEN mhlen;

		SQLBindCol(hstmt, 1, SQL_C_LONG, &monstercode, sizeof(monstercode), &mcodelen);
		SQLBindCol(hstmt, 2, SQL_C_LONG, &pos.x, sizeof(pos.x), &posxlen);
		SQLBindCol(hstmt, 2, SQL_C_LONG, &pos.y, sizeof(pos.y), &posylen);
		SQLBindCol(hstmt, 2, SQL_C_LONG, &maxhealth, sizeof(maxhealth), &mhlen);

		retCode = SQLFetch(hstmt);

		while (retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO)
		{
			MakeMonsterDropTable(monstercode);
			ret.emplace_back(monstercode, maxhealth, pos);

			retCode = SQLFetch(hstmt);
		}

		ReleaseHandle(hstmt);

		return ret;
	}

	bool ReserveNickname(std::string& nickname_)
	{
		bool bRet = m_RedisManager.ReserveNickname(nickname_);

		// Redis에 예약정보 남기기 실패 (다른 유저가 이미 예약중)
		if (bRet == false)
		{
			return false;
		}

		// 유효한 닉네임이 아님.
		if (!CheckNicknameIsValid(nickname_))
		{
			m_RedisManager.CancelReserveNickname(nickname_);
			return false;
		}

		HANDLE hstmt = m_Pool->Allocate();

		if (hstmt == INVALID_HANDLE_VALUE)
		{
			m_RedisManager.CancelReserveNickname(nickname_);
			return false;
		}

		//std::wstring wnick = std::wstring(nickname_.begin(), nickname_.end());

		SQLWCHAR query[128] = { 0 };
		swprintf((wchar_t*)query, 128,
			L"SELECT 1 AS RES "
			L"FROM CHARINFO "
			L"WHERE CHARNAME = N'%s' "
			, nickname_.c_str());

		if (DB_DEBUG)
		{
			//std::cout << "Database::ReserveNickname : nick : " << nickname_ << '\n';
			//std::wcout << "Database::ReserveNickname : query : " << query << '\n';
		}

		SQLRETURN retCode = SQLExecDirect(hstmt, query, SQL_NTS);

		// SQLExecute Failed
		if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO)
		{
			ReleaseHandle(hstmt);
			m_RedisManager.CancelReserveNickname(nickname_);
			std::cerr << "Database::ReserveNickname : Failed to Execute\n";
			return false;
		}

		// 데이터베이스에서 해당 닉네임이 검색됨
		if (SQLFetch(hstmt) == SQL_SUCCESS)
		{
			ReleaseHandle(hstmt);
			m_RedisManager.CancelReserveNickname(nickname_);
			return false;
		}
		// 검색된 닉네임이 없음.
		else
		{
			ReleaseHandle(hstmt);
			return true;
		}
	}

	// ReserveNickname이 성공한 유저인지 확인하고 호출할 것.
	bool CancelReserveNickname(std::string& nickname_)
	{
		return m_RedisManager.CancelReserveNickname(nickname_);
	}

	// 생성에 성공했다면 charno를 리턴.
	// 생성에 실패했다면 -1을 리턴.
	int CreateCharactor(const int usercode_, std::string& nickname_, const int startMapcode_)
	{
		if (!InsertCharInfo(nickname_, startMapcode_))
		{
			std::cerr << "DB::CreateCharactor : Failed to Insert CharInfo\n";
			return -1;
		}
		int iRet = GetCharNo(nickname_);

		if (iRet <= 0)
		{
			std::cerr << "DB::CreateCharactor : Failed to Get CharNo\n";
			return -1;
		}

		if (!InsertCharList(usercode_, iRet))
		{
			std::cerr << "DB::CreateCharactor : Failed to Create CharList\n";
			return -1;
		}

		if (!CreateInventory(iRet))
		{
			std::cerr << "DB::CreateCharactor : Failed to Create Inven\n";
			return -1;
		}

		return iRet;
	}

	bool GetInventory(const int charcode_, std::string& out_)
	{
		std::string str;

		// CacheServer 먼저 확인
		if (m_RedisManager.GetInven(charcode_, str))
		{
			out_ = str;
			return true;
		}

		HANDLE hstmt = m_Pool->Allocate();

		SQLWCHAR query[64] = { 0 };
		swprintf((wchar_t*)query, 64,
			L"SELECT INVEN "
			L"FROM CHARINVEN "
			L"WHERE CHARNO = %d; "
			, charcode_);

		if (DB_DEBUG)
		{
			std::wcout << "Database::GetInventory : query : " << query << '\n';
		}

		SQLRETURN retCode = SQLExecDirect(hstmt, query, SQL_NTS);

		// SQLExecute Failed
		if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO)
		{
			std::wcerr << L"Database::GetInventory : Failed to Execute\n";
			ReleaseHandle(hstmt);
			return false;
		}

		retCode = SQLFetch(hstmt);
		if (retCode != SQL_SUCCESS)
		{
			std::cerr << "DB::GetInventory : Failed to Fetch\n";
			ReleaseHandle(hstmt);
			return false;
		}

		SQLCHAR cInven[8000] = { 0 };
		SQLLEN len;

		retCode = SQLGetData(hstmt, 1, SQL_C_CHAR, cInven, sizeof(cInven), &len);

		if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO)
		{
			std::cerr << "DB::GetInventory : Failed to Fetch\n";
			ReleaseHandle(hstmt);
			return false;
		}

		ReleaseHandle(hstmt);

		out_ = std::string(reinterpret_cast<char*>(cInven));

		if (DB_DEBUG)
		{
			std::cout << "DB::GetInventory : out : " << out_ << "\n";
		}

		// 레디스에 등록하기
		if (!m_RedisManager.CreateInven(charcode_, out_))
		{
			std::wcerr << L"Database::GetInventory : 레디스에 정보 등록 실패\n";
			return false;
		}

		return true;
	}

	bool UpdateInventorySlot(const int charcode_, int idx_, Item& item_)
	{
		if (idx_ < 0 || idx_ >= MAX_SLOT)
		{
			return false;
		}
	}

	std::string GetSalesList(const int npcCode_)
	{
		std::string ret;

		// 레디스 먼저 확인.
		if (m_RedisManager.GetSalesList(npcCode_, ret))
		{
			return ret;
		}

		auto pair = m_SalesList.Get(npcCode_);

		// 해당하는 값 없음
		if (pair.first == pair.second)
		{
			std::cerr << "DB::GetSalesList : No SalesList on" << npcCode_ << "\n";
			return std::string();
		}

		std::map<int, int> item_price_map;

		for (auto itr = pair.first; itr != pair.second; itr++)
		{
			int itemcode = itr->second;
			int price = m_PriceTable.Get(itemcode);
			item_price_map.emplace(itemcode, price);
		}

		if (!m_JsonMaker.ToJsonString(item_price_map, ret))
		{
			std::cerr << "DB::GetSalesList : Failed To Make JsonStr\n";
			return std::string();
		}

		if (!m_RedisManager.CreateSalesList(npcCode_, ret))
		{
			std::cerr << "DB::GetSalesList : Failed To Insert on Redis\n";
		}

		return ret;
	}

	bool DebugSetGold(const int charcode_)
	{
		REDISRETURN eRet;


		while ((eRet = m_RedisManager.SetGold(charcode_)) == REDISRETURN::LOCK_FAILED)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(0));
		}

		if (eRet != REDISRETURN::SUCCESS)
		{
			return false;
		}

		return true;
	}

private:
	bool CheckIDIsValid(const std::string& id_)
	{
		if (id_.length() > 10 || id_.length() < 3)
		{
			return false;
		}

		// 영문 대소문자, 숫자만 허용
		for (size_t i = 0; i < id_.length(); i++)
		{
			if (id_[i] > 'z' ||
				(id_[i] < 'a' && id_[i] > 'Z') ||
				(id_[i] < 'A' && id_[i] > '9') ||
				id_[i] < '0')
			{
				return false;
			}
		}

		return true;
	}

	bool CheckPWIsValid(const std::string& pw_)
	{
		if (pw_.length() > 10 || pw_.length() < 3)
		{
			return false;
		}

		// 영문 대소문자, 숫자만 허용
		for (size_t i = 0; i < pw_.length(); i++)
		{
			if (pw_[i] > 'z' ||
				(pw_[i] < 'a' && pw_[i] > 'Z') ||
				(pw_[i] < 'A' && pw_[i] > '9') ||
				pw_[i] < '0')
			{
				return false;
			}
		}

		return true;
	}

	bool CheckNicknameIsValid(const std::string& nickname)
	{
		if (nickname.length() > 10 || nickname.length() < 3)
		{
			return false;
		}

		if (hasInvalidCh(nickname))
		{
			return false;
		}

		return true;
	}

	// '닼' 같은 문자는 unicode조합이 된 문자고 euc-kr에서는 한글로 인식되지 않을 수 있다.
	bool isHangul(unsigned char byte1, unsigned char byte2) {
		// EUC-KR의 한글 범위는 첫 바이트가 0xA1 ~ 0xFE, 두 번째 바이트는 0x41 ~ 0x5A
		unsigned short combined = (byte1 << 8) | byte2;

		// U+AC00 ~ U+D7A3 범위 확인
		return (combined >= 0xAC00 && combined <= 0xD7A3);
	}

	bool isAlpha(unsigned char ch)
	{
		return std::isalpha(ch);
	}

	bool isDigit(unsigned char ch)
	{
		return std::isdigit(ch);
	}

	bool hasInvalidCh(const std::string& nickname)
	{
		size_t i = 0;
		while (i < nickname.length()) {
			unsigned char byte1 = static_cast<unsigned char>(nickname[i]);

			// 2바이트 문자 (EUC-KR에서 한글)
			if (byte1 >= 0xA1 && byte1 <= 0xFE && i + 1 < nickname.length()) {
				unsigned char byte2 = static_cast<unsigned char>(nickname[i + 1]);

				// 한글인지 확인
				if (!isHangul(byte1, byte2)) {
					return true;  // 한글이 아닌 2바이트 문자
				}
				i += 2;  // 한글은 2바이트이므로 2칸 이동
			}
			// 1바이트 문자 (영문, 숫자, 특수문자 등)
			else {
				if (!(isAlpha(byte1) || isDigit(byte1))) {
					return true;  // 영문/숫자가 아닌 문자가 있음
				}
				i++;  // 1바이트 문자일 경우 1칸 이동
			}
		}
		return false;  // 모든 문자가 허용된 문자일 경우
	}

	// 레디스의 SetNx를 이용, 해당 유저코드로 로그인한 유저가 없다면 등록한다.
	// true : success to SignIn
	// false : fail to SignIn (Already In Use)
	bool TrySignIn(const int usercode_, const std::string& ip_)
	{
		return m_RedisManager.MakeSession(usercode_, ip_);
	}

	// 사용한 구문핸들의 상태를 초기화하고 재사용할 수 있도록 커넥션풀에 넣는다.
	void ReleaseHandle(HANDLE stmt_)
	{
		//SQLCloseCursor(stmt_);
		SQLFreeStmt(stmt_, SQL_UNBIND); // BIND COL 해제
		SQLFreeStmt(stmt_, SQL_CLOSE); // 커서 닫고 보류된 결과 해제
		SQLFreeStmt(stmt_, SQL_RESET_PARAMS); // BIND PARAM 해제
		m_Pool->Deallocate(stmt_); // 커넥션풀 반환

		return;
	}

	bool InjectionCheck(const std::string& str_)
	{
		// 특수기호 제거
		for (int i = 0; i < str_.size(); i++)
		{
			if (str_[i] >= '0' && str_[i] <= '9')
			{
				continue;
			}

			if (str_[i] >= 'A' && str_[i] <= 'Z')
			{
				continue;
			}

			if (str_[i] >= 'a' && str_[i] <= 'z')
			{
				continue;
			}

			return false;
		}

		// 예약어 제거

		for (const std::string& word : m_ReservedWord)
		{
			if (IsStringContains(str_, word))
			{
				return false;
			}
		}

		return true;
	}

	// Todo. RabinKarf로 대체하자.
	bool IsStringContains(const std::string& orgstr_, const std::string& somestr_)
	{
		size_t strsize1 = orgstr_.size(), strsize2 = somestr_.size();

		for (size_t i = 0; i < strsize1; i++)
		{
			// 더 이상 찾을 수 없음
			if (i + strsize2 > strsize1)
			{
				return false;
			}

			if (orgstr_[i] == somestr_[0])
			{
				// 같은 문자열인가
				bool check = true;

				for (size_t j = 1; j < strsize2; j++)
				{
					// 같은 문자열이 아님
					if (orgstr_[i + j] != somestr_[j])
					{
						check = false;
						break;
					}
				}

				if (check)
				{
					return true;
				}
			}
		}

		return false;
	}

	void MakeReservedWordList()
	{
		m_ReservedWord.clear();

		// 예약어 : OR, SELECT, INSERT, DELETE, UPDATE, CREATE, DROP, EXEC, UNION, FETCH, DECLARE, TRUNCATE
		m_ReservedWord.push_back("OR");
		m_ReservedWord.push_back("SELECT");
		m_ReservedWord.push_back("INSERT");
		m_ReservedWord.push_back("DELETE");
		m_ReservedWord.push_back("UPDATE");
		m_ReservedWord.push_back("CREATE");
		m_ReservedWord.push_back("DROP");
		m_ReservedWord.push_back("EXEC");
		m_ReservedWord.push_back("UNION");
		m_ReservedWord.push_back("FETCH");
		m_ReservedWord.push_back("DECLARE");
		m_ReservedWord.push_back("TRUNCATE");

		return;
	}

	void MakePriceTable()
	{
		// MSSQL에서 ITEMTABLE에 있는 정보 긁어오기.
		// T(ITEMTABLE) : [ITEMCODE][PURCHASEPRICE]
		// 일단은 아이템이 많지는 않을거 같은데 10MB를 넘어가면 10MB단위로 잘라서 가져오는 것이 좋다고 한다.
		// 일단은 행별 데이터는 8byte로 설정되어 있다.

		HANDLE hstmt = m_Pool->Allocate();

		if (hstmt == INVALID_HANDLE_VALUE)
		{
			std::cerr << "DB::MakePriceTable : 핸들 발급 실패\n";
			return;
		}

		SQLPrepare(hstmt, (SQLWCHAR*)
			L"SELECT ITEMCODE, PURCHASEPRICE "
			L"FROM ITEMTABLE", SQL_NTS);

		SQLRETURN retCode = SQLExecute(hstmt);

		// SQLExecute Failed
		if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO)
		{
			ReleaseHandle(hstmt);
			std::cerr << "Database::MakePriceTable : Failed to Execute\n";
			return;
		}

		int itemcode;
		SQLLEN itemcodeLen;
		int purchasePrice;
		SQLLEN purchasePriceLen;

		SQLBindCol(hstmt, 1, SQL_C_LONG, &itemcode, sizeof(itemcode), &itemcodeLen);
		SQLBindCol(hstmt, 2, SQL_C_LONG, &purchasePrice, sizeof(purchasePrice), &purchasePriceLen);

		retCode = SQLFetch(hstmt);

		while (retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO)
		{
			m_PriceTable.Insert(itemcode, purchasePrice);

			retCode = SQLFetch(hstmt);
		}

		ReleaseHandle(hstmt);

		return;
	}
	
	void MakeSalesList()
	{
		// T(SALESLIST) : [NPCCODE] [ITEMCODE]

		HANDLE hstmt = m_Pool->Allocate();

		if (hstmt == INVALID_HANDLE_VALUE)
		{
			std::cerr << "DB::MakeSalesList : 핸들 발급 실패\n";
			return;
		}

		SQLPrepare(hstmt, (SQLWCHAR*)
			L"SELECT NPCCODE, ITEMCODE "
			L"FROM SALESLIST", SQL_NTS);

		SQLRETURN retCode = SQLExecute(hstmt);

		// SQLExecute Failed
		if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO)
		{
			ReleaseHandle(hstmt);
			std::cerr << "Database::MakeSalesList : Failed to Execute\n";
			return;
		}

		int npccode;
		SQLLEN npccodeLen;
		int itemcode;
		SQLLEN itemcodeLen;

		SQLBindCol(hstmt, 1, SQL_C_LONG, &npccode, sizeof(npccode), &npccodeLen);
		SQLBindCol(hstmt, 2, SQL_C_LONG, &itemcode, sizeof(itemcode), &itemcodeLen);

		retCode = SQLFetch(hstmt);

		while (retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO)
		{
			if (DB_DEBUG)
			{
				std::cout << "npc : " << npccode << ", item : " << itemcode << "\n";
			}
			
			m_SalesList.Insert(npccode, itemcode);

			retCode = SQLFetch(hstmt);
		}

		ReleaseHandle(hstmt);

		return;
	}

	void MakeMapEdgeList()
	{
		// T(MAP_EDGE) : [FROM_MAP_CODE] [TO_MAP_CODE]

		HANDLE hstmt = m_Pool->Allocate();

		if (hstmt == INVALID_HANDLE_VALUE)
		{
			std::cerr << "DB::MakeMapEdgeList : 핸들 발급 실패\n";
			return;
		}

		SQLWCHAR query[64] = 	L"SELECT FROM_MAP_CODE, TO_MAP_CODE "
								L"FROM MAP_EDGE ";

		SQLRETURN retCode = SQLExecDirect(hstmt, query, SQL_NTS);

		// SQLExecute Failed
		if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO)
		{
			PrintError(hstmt);

			ReleaseHandle(hstmt);
			std::cerr << "Database::MakeMapEdgeList : Failed to Execute\n";
			return;
		}

		int fromcode;
		SQLLEN fromcodeLen;
		int tocode;
		SQLLEN tocodeLen;

		retCode = SQLFetch(hstmt);

		while (retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO)
		{
			SQLGetData(hstmt, 1, SQL_C_LONG, &fromcode, 0, &fromcodeLen);
			SQLGetData(hstmt, 2, SQL_C_LONG, &tocode, 0, &tocodeLen);

			m_MapEdge.Insert(fromcode, tocode);

			retCode = SQLFetch(hstmt);
		}

		ReleaseHandle(hstmt);

		return;
	}

	void MakeMapNPCInfo()
	{
		// T(MAPNPCINFO) : [MAPCODE][NPCCODE]

		HANDLE hstmt = m_Pool->Allocate();

		if (hstmt == INVALID_HANDLE_VALUE)
		{
			std::cerr << "DB::MakeMapNPCInfo : 핸들 발급 실패\n";
			return;
		}

		SQLPrepare(hstmt, (SQLWCHAR*)
			L"SELECT MAPCODE, NPCCODE "
			L"FROM MAPNPCINFO", SQL_NTS);

		SQLRETURN retCode = SQLExecute(hstmt);

		// SQLExecute Failed
		if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO)
		{
			ReleaseHandle(hstmt);
			std::cerr << "Database::MakeMapNPCInfo : Failed to Execute\n";
			return;
		}

		int mapcode;
		SQLLEN mapcodeLen;
		int npccode;
		SQLLEN npccodeLen;

		SQLBindCol(hstmt, 1, SQL_C_LONG, &mapcode, sizeof(mapcode), &mapcodeLen);
		SQLBindCol(hstmt, 2, SQL_C_LONG, &npccode, sizeof(npccode), &npccodeLen);

		retCode = SQLFetch(hstmt);

		while (retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO)
		{
			m_MapNPCInfo.Insert(mapcode, npccode);

			retCode = SQLFetch(hstmt);
		}

		ReleaseHandle(hstmt);

		return;
	}

	// 캐릭터 정보를 DB에 생성한다.
	bool InsertCharInfo(std::string& charname_, const int startMapcode_)
	{
		HANDLE hstmt = m_Pool->Allocate();

		if (hstmt == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		std::wstring wnick = convertEucKrToUtf16(charname_);

		SQLWCHAR query[512] = { 0 };
		swprintf((wchar_t*)query, 512,
			L"INSERT INTO CHARINFO (CHARNAME, CLASS_CODE, LV, EXPERIENCE, STAT_POINT, "
			L"HEALTH_POINT, MANA_POINT, CURRENT_HEALTH, CURRENT_MANA, STRENGTH, DEXTERITY, "
			L"INTELLIGENCE, MENTALITY, GOLD, LASTMAPCODE) "
			L"VALUES (N'%s', 1, 1, 0, 0, 100, 100, 100, 100, 4, 4, 4, 4, 0, %d) "
			, wnick.c_str(), startMapcode_);

		if (DB_DEBUG)
		{
			//std::cout << "Database::InsertCharInfo : nick : " << charname_ << '\n';
			//std::wcout << "Database::InsertCharInfo : query : " << query << '\n';
		}

		SQLRETURN retcode = SQLExecDirect(hstmt, query, SQL_NTS);
		
		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
		{
			PrintError(hstmt);
			std::cerr << "DB::InsertCharInfo : Failed to Execute\n";
			ReleaseHandle(hstmt);
			return false;
		}

		ReleaseHandle(hstmt);
		return true;
	}

	// 생성한 캐릭터의 CHARNO를 받아온다.
	int GetCharNo(std::string& charname_)
	{
		HANDLE hstmt = m_Pool->Allocate();

		if (hstmt == INVALID_HANDLE_VALUE)
		{
			return -1;
		}

		std::wstring wnick = convertEucKrToUtf16(charname_);

		SQLWCHAR query[512] = { 0 };
		swprintf((wchar_t*)query, 512,
			L"SELECT CHARNO "
			L"FROM CHARINFO "
			L"WHERE CHARNAME = N'%s' "
			, wnick.c_str());

		if (DB_DEBUG)
		{
			std::wcout << L"Database::GetCharNo : query : " << query << L'\n';
		}

		SQLRETURN retcode = SQLExecDirect(hstmt, query, SQL_NTS);

		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
		{
			std::cerr << "DB::GetCharNo : Failed to Execute\n";
			ReleaseHandle(hstmt);
			return -1;
		}

		int ret = 0;
		SQLLEN len;
		retcode = SQLBindCol(hstmt, 1, SQL_C_LONG, &ret, sizeof(ret), &len);

		if (retcode != SQL_SUCCESS)
		{
			std::cerr << "DB::GetCharNo : Failed to BindCol\n";
			ReleaseHandle(hstmt);
			return -1;
		}

		retcode = SQLFetch(hstmt);
		if (retcode != SQL_SUCCESS)
		{
			std::cerr << "DB::GetCharNo : Failed to Fetch\n";
			ReleaseHandle(hstmt);
			return -1;
		}

		ReleaseHandle(hstmt);
		return ret;
	}

	// USERNO와 CHARNO를 연결하는 정보를 DB에 추가한다.
	bool InsertCharList(const int usercode_, const int charcode_)
	{
		HANDLE hstmt = m_Pool->Allocate();

		if (hstmt == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		SQLWCHAR query[512] = { 0 };
		swprintf((wchar_t*)query, 512,
			L"INSERT INTO CHARLIST (USERNO, CHARNO) VALUES (%d, %d) "
			, usercode_, charcode_);

		if (DB_DEBUG)
		{
			std::wcout << "Database::InsertCharList : query : " << query << '\n';
		}

		SQLRETURN retcode = SQLExecDirect(hstmt, query, SQL_NTS);


		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
		{
			std::cerr << "DB::InsertCharList : Failed to Execute\n";
			ReleaseHandle(hstmt);
			return false;
		}

		ReleaseHandle(hstmt);
		return true;
	}

	bool CreateInventory(const int charcode_)
	{
		HANDLE hstmt = m_Pool->Allocate();

		if (hstmt == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		Inventory inven;

		std::string strInven;
		m_JsonMaker.ToJsonString(inven, strInven);
		std::wstring wstrInven = std::wstring(strInven.begin(), strInven.end());

		if (DB_DEBUG)
		{
			//std::wcout << "DB::CreateInventory : invenstr : " << wstrInven << "\n";
		}

		SQLWCHAR query[4096] = { 0 };
		if (swprintf((wchar_t*)query, 4096,
			L"INSERT INTO CHARINVEN (CHARNO, INVEN) VALUES (%d, '%s') "
			, charcode_, wstrInven.c_str()) == -1)
		{
			std::cerr << "DB::CreateInventory : swprintf Failed.\n";
			return false;
		}

		if (DB_DEBUG)
		{
			//std::wcout << "Database::CreateInventory : query : " << query << '\n';
		}

		SQLRETURN retcode = SQLExecDirect(hstmt, query, SQL_NTS);

		if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
		{
			PrintError(hstmt);
			std::cerr << "DB::CreateInventory : Failed to Execute\n";
			ReleaseHandle(hstmt);
			return false;
		}

		ReleaseHandle(hstmt);
		return true;
	}

	// 사용가능성이 있는 몬스터에 대한 드랍정보를 가져와 기록한다.
	// 몬스터가 정말 아무것도 드랍하지 않더라도 DB에 Gold를 드랍할 확률을 적는다. -> 해당 몬스터 정보가 있는지 판단하는데 쓸 것.
	void MakeMonsterDropTable(const int monsterCode_)
	{
		// T(DROPINFO) [MONSTERCODE] [ITEMCODE] [NUM] [DEN]
		// NUM / DEN = 드랍확률

		auto range = m_DropInfo.Get(monsterCode_);
		// 정보 있음 (begin() != end())
		if (range.first != range.second)
		{
			return;
		}

		HANDLE hstmt = m_Pool->Allocate();

		if (hstmt == INVALID_HANDLE_VALUE)
		{
			std::cerr << "DB::MakeMonsterDropTable : 핸들 발급 실패\n";
			return;
		}

		SQLPrepare(hstmt, (SQLWCHAR*)
			L"SELECT ITEMCODE, NUM, DEN "
			L"FROM DROPINFO "
			L"WHERE MONSTERCODE = ? ", SQL_NTS);

		int param = monsterCode_;
		SQLLEN paramlen;

		SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &param, 0, &paramlen);

		SQLRETURN retCode = SQLExecute(hstmt);

		// SQLExecute Failed
		if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO)
		{
			ReleaseHandle(hstmt);
			std::cerr << "Database::MakePriceTable : Failed to Execute\n";
			return;
		}

		int itemcode;
		int num;
		int den;

		SQLLEN icodelen;
		SQLLEN numlen;
		SQLLEN denlen;

		SQLBindCol(hstmt, 1, SQL_C_LONG, &itemcode, sizeof(itemcode), &icodelen);
		SQLBindCol(hstmt, 2, SQL_C_LONG, &num, sizeof(num), &numlen);
		SQLBindCol(hstmt, 2, SQL_C_LONG, &den, sizeof(den), &denlen);

		retCode = SQLFetch(hstmt);

		while (retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO)
		{
			m_DropInfo.Insert(monsterCode_, MonsterDropInfo(itemcode, num, den));

			retCode = SQLFetch(hstmt);
		}

		ReleaseHandle(hstmt);

		return;

	}

	void PrintError(SQLHANDLE hstmt)
	{
		SQLWCHAR sqlState[6] = { 0 };
		SQLWCHAR message[1000] = { 0 };
		SQLINTEGER nativeError;
		SQLSMALLINT msgLength;

		// 진단 메시지 가져오기
		SQLRETURN ret = SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, 1, sqlState, &nativeError, message, sizeof(message), &msgLength);

		if (SQL_SUCCEEDED(ret)) {
			std::wcout << L"SQLState: " << sqlState << L"\n";
			std::wcout << L"Native Error Code: " << nativeError << L"\n";
			std::wcout << L"Error Message: " << message << L"\n";
		}
		else {
			std::cout << "No error information available.\n";
		}
	}

	std::wstring convertEucKrToUtf16(const std::string& eucKrStr) {
		// EUC-KR(CP949) -> UTF-16 변환을 위한 버퍼 크기 계산
		int utf16Length = MultiByteToWideChar(CP_ACP, 0, eucKrStr.c_str(), -1, NULL, 0);

		if (utf16Length == 0) {
			std::wcerr << L"Conversion failed!" << std::endl;
			return L"";
		}

		std::wstring utf16Str(utf16Length, L'\0');
		MultiByteToWideChar(CP_ACP, 0, eucKrStr.c_str(), -1, &utf16Str[0], utf16Length);

		return utf16Str;
	}

	std::string convertUtf16ToEucKr(const std::wstring& utf16)
	{
		int Length = WideCharToMultiByte(CP_ACP, 0, utf16.c_str(), -1, NULL, 0, NULL, NULL);

		if (Length == 0) {
			std::wcerr << L"Conversion failed!" << std::endl;
			return std::string();
		}

		std::string euckr(Length, '\0');
		WideCharToMultiByte(CP_ACP, 0, utf16.c_str(), -1, &euckr[0], Length, NULL, NULL);

		return euckr;
	}

	RedisManager m_RedisManager;
	JsonMaker m_JsonMaker;

	// [itemcode][price]
	UMapWrapper<int, int> m_PriceTable;

	// [npccode][itemcode]
	UMultiMapWrapper<int, int> m_SalesList;

	// [mapcode][mapcode]
	// 첫번째 파라미터의 맵으로부터 두번째파라미터의 맵으로 이동 가능함을 표시.
	UMultiMapWrapper<int, int> m_MapEdge;

	// [mapcode][npccode]
	// 해당 맵에 해당 npc가 존재함을 표시.
	UMultiMapWrapper<int, int> m_MapNPCInfo;

	// [monstercode][DropInfo(itemcode, num, den)]
	// 해당 몬스터가 itemcode의 아이템을 num / den 확률로 떨어뜨림을 표시.
	UMultiMapWrapper<int, MonsterDropInfo> m_DropInfo;

	// [monstercode][Experience]
	// 해당 몬스터를 쓰러뜨리면 얻는 경험치를 표시.
	UMapWrapper<int, int> m_MonsterEXPInfo;

	// [monstercode][Gold]
	// 해당 몬스터를 쓰러뜨리면 얻는 평균 골드를 표시.
	UMapWrapper<int, int> m_MonsterGoldInfo;

	std::vector<std::string> m_ReservedWord;
	std::unique_ptr<MSSQL::ConnectionPool> m_Pool;
	std::unique_ptr<MemoryPool<CharList>> m_CharListPool;
	std::unique_ptr<MemoryPool<CharInfo>> m_CharInfoPool;
};