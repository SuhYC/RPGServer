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
		MakeMapMonsterInfo();
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

		SQLPrepare(hstmt, (SQLWCHAR*)
			L"IF NOT EXISTS (SELECT 1 FROM LOGINDATA WHERE ID = ?) "
			L"BEGIN "
			L"    INSERT INTO LOGINDATA (USERID, PASSWORD, QUESTNO, HINT, ANSWER) VALUES (?, ?, ?, ?, ?); "
			L"    SELECT 'S' AS Result; "
			L"END "
			L"ELSE "
			L"BEGIN "
			L"    SELECT 'F' AS Result; "
			L"END", SQL_NTS);

		SQLLEN idLength = id_.length();
		SQLLEN pwLength = pw_.length();
		SQLLEN hintLength = hint_.length();
		SQLLEN ansLength = ans_.length();

		int questno = questno_;

		SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (SQLCHAR*)id_.c_str(), 0, &idLength);
		SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (SQLCHAR*)id_.c_str(), 0, &idLength);
		SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (SQLCHAR*)pw_.c_str(), 0, &pwLength);
		SQLBindParameter(hstmt, 4, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &questno, 0, NULL);
		SQLBindParameter(hstmt, 5, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (SQLCHAR*)hint_.c_str(), 0, &hintLength);
		SQLBindParameter(hstmt, 6, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (SQLCHAR*)ans_.c_str(), 0, &ansLength);

		SQLRETURN retCode = SQLExecute(hstmt);

		// SQLExecute Failed
		if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO)
		{
			ReleaseHandle(hstmt);
			std::cerr << "Database::SIGNUP : Failed to Execute\n";
			return eReturnCode::SYSTEM_ERROR;
		}

		SQLCHAR result_message[2]; // 'S' or 'F'
		SQLLEN resultlen;
		retCode = SQLFetch(hstmt);

		// SQLFetch Failed
		if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO)
		{
			ReleaseHandle(hstmt);
			std::cerr << "Database::SIGNUP : Failed to Fetch\n";
			return eReturnCode::SYSTEM_ERROR;
		}

		SQLRETURN dataRet = SQLGetData(hstmt, 1, SQL_C_CHAR, result_message, sizeof(result_message) / sizeof(result_message[0]), &resultlen);
				
		// SQLGetData Failed
		if (dataRet != SQL_SUCCESS && dataRet != SQL_SUCCESS_WITH_INFO)
		{
			ReleaseHandle(hstmt);
			std::wcerr << L"Database::SIGNUP : Failed to Get Data\n";
			return eReturnCode::SYSTEM_ERROR;
		}

		
		if (DB_DEBUG)
		{
			std::wcout << result_message[0] << "\n";
		}

		if (result_message[0] == 'S')
		{
			ReleaseHandle(hstmt);
			return eReturnCode::SIGNUP_SUCCESS;
		}
		// 'F' -> Failed to INSERT : Already In Use
		else
		{
			ReleaseHandle(hstmt);
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

		SQLLEN idLength = id_.length();
		SQLLEN pwLength = pw_.length();

		SQLPrepare(hstmt, (SQLWCHAR*)
			L"SELECT USERCODE FROM LOGINDATA WHERE ID = ? AND PASSWORD = ?; "
			, SQL_NTS);

		SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR, 0, 0, (SQLCHAR*)id_.c_str(), 0, &idLength);
		SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR, 0, 0, (SQLCHAR*)pw_.c_str(), 0, &pwLength);

		SQLRETURN retCode = SQLExecute(hstmt);

		// SQLExecute Failed
		if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO)
		{
			std::wcerr << L"DB::SIGNIN : Failed to Execute\n";

			ReleaseHandle(hstmt);
			return CLIENT_NOT_CERTIFIED;
		}

		long result = 0;
		retCode = SQLFetch(hstmt);

		// CLIENT_NOT_CERTIFIED : WRONG ID OR PW
		if (retCode == SQL_NO_DATA)
		{
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

		SQLLEN num_rows;
		SQLRETURN dataRet = SQLGetData(hstmt, 1, SQL_C_LONG, &result, sizeof(result), &num_rows);
				
		// SQLGetData Failed
		if (dataRet != SQL_SUCCESS && dataRet != SQL_SUCCESS_WITH_INFO)
		{
			std::wcerr << L"DB::SIGNIN : Failed to Get Data\n";
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

		SQLPrepare(hstmt, (SQLWCHAR*)
			L"SELECT CHARNAME, LV, EXPERIENCE, STATPOINT, HEALTHPOINT, MANAPOINT, STRENGTH, DEXTERITY, INTELLIGENCE, MENTALITY, GOLD, LASTMAPCODE FROM CHARINFO WHERE CHARNO = ?;",
			SQL_NTS);

		int paramValue = charNo_;
		SQLLEN paramValueLength = 0;

		SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &paramValue, 0, &paramValueLength);

		SQLRETURN retCode = SQLExecute(hstmt);

		// SQLExecute Failed
		if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO)
		{
			std::wcerr << L"Database::GetCharInfo : Failed to Execute\n";
			ReleaseHandle(hstmt);
			return std::string();
		}

		retCode = SQLFetch(hstmt);

		// SQLFetch Failed
		if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO)
		{
			std::wcerr << L"Database::GetCharInfo : Failed to Fetch\n";
			ReleaseHandle(hstmt);
			return std::string();
		}

		CharInfo* pCharInfo = m_CharInfoPool->Allocate();

		if (pCharInfo == nullptr)
		{
			std::cerr << "Database::GetCharInfo : Failed to Allocate Info Instance\n";
			ReleaseHandle(hstmt);
			return std::string();
		}

		SQLLEN len;
		SQLGetData(hstmt, 1, SQL_C_WCHAR, pCharInfo->CharName, sizeof(pCharInfo->CharName) / sizeof(pCharInfo->CharName[0]), &len);
		SQLGetData(hstmt, 2, SQL_C_LONG, &(pCharInfo->Level), 0, &len);
		SQLGetData(hstmt, 3, SQL_C_LONG, &(pCharInfo->Experience), 0, &len);
		SQLGetData(hstmt, 4, SQL_C_LONG, &(pCharInfo->StatPoint), 0, &len);
		SQLGetData(hstmt, 5, SQL_C_LONG, &(pCharInfo->HealthPoint), 0, &len);
		SQLGetData(hstmt, 6, SQL_C_LONG, &(pCharInfo->ManaPoint), 0, &len);
		SQLGetData(hstmt, 7, SQL_C_LONG, &(pCharInfo->Strength), 0, &len);
		SQLGetData(hstmt, 8, SQL_C_LONG, &(pCharInfo->Dexterity), 0, &len);
		SQLGetData(hstmt, 9, SQL_C_LONG, &(pCharInfo->Intelligence), 0, &len);
		SQLGetData(hstmt, 10, SQL_C_LONG, &(pCharInfo->Mentality), 0, &len);
		SQLGetData(hstmt, 11, SQL_C_LONG, &(pCharInfo->Gold), 0, &len);
		SQLGetData(hstmt, 12, SQL_C_LONG, &(pCharInfo->LastMapCode), 0, &len);

		ReleaseHandle(hstmt);

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

		SQLPrepare(hstmt, (SQLWCHAR*)
			L"SELECT CHARNAME, LV, EXPERIENCE, STATPOINT, HEALTHPOINT, MANAPOINT, STRENGTH, DEXTERITY, INTELLIGENCE, MENTALITY, GOLD, LASTMAPCODE"
			L"FROM CHARINFO"
			L"WHERE CHARNO = ?;", SQL_NTS);

		int paramValue = charNo_;
		SQLLEN paramValueLength = 0;

		SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &paramValue, 0, &paramValueLength);

		SQLRETURN retCode = SQLExecute(hstmt);

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
		SQLGetData(hstmt, 1, SQL_C_WCHAR, ret->CharName, sizeof(ret->CharName) / sizeof(ret->CharName[0]), &len);
		SQLGetData(hstmt, 2, SQL_C_LONG, &(ret->Level), 0, &len);
		SQLGetData(hstmt, 3, SQL_C_LONG, &(ret->Experience), 0, &len);
		SQLGetData(hstmt, 4, SQL_C_LONG, &(ret->StatPoint), 0, &len);
		SQLGetData(hstmt, 5, SQL_C_LONG, &(ret->HealthPoint), 0, &len);
		SQLGetData(hstmt, 6, SQL_C_LONG, &(ret->ManaPoint), 0, &len);
		SQLGetData(hstmt, 7, SQL_C_LONG, &(ret->Strength), 0, &len);
		SQLGetData(hstmt, 8, SQL_C_LONG, &(ret->Dexterity), 0, &len);
		SQLGetData(hstmt, 9, SQL_C_LONG, &(ret->Intelligence), 0, &len);
		SQLGetData(hstmt, 10, SQL_C_LONG, &(ret->Mentality), 0, &len);
		SQLGetData(hstmt, 11, SQL_C_LONG, &(ret->Gold), 0, &len);
		SQLGetData(hstmt, 12, SQL_C_LONG, &(ret->LastMapCode), 0, &len);

		ReleaseHandle(hstmt);

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

	// CharList Class 만들어야함 + 메모리풀도 -> 패킷데이터랑 템플릿으로 묶을까?
	// CharNo 여러개를 담아 온다.
	// CharList는 Redis에 담지 않는다. (유저 요청이 접속당 1번수준. 추후 적절한 유저가 캐릭터선택을 요청하는지는 확인하게 만들어야할까?)
	std::string GetCharList(const int userCode_)
	{
		HANDLE hstmt = m_Pool->Allocate();
		SQLPrepare(hstmt, (SQLWCHAR*)
			L"SELECT CHARNO FROM CHARLIST WHERE USERNO = ?;",
			SQL_NTS);

		int paramValue = userCode_;
		SQLLEN paramLen = 0;

		SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &paramValue, 0, &paramLen);

		SQLRETURN retCode = SQLExecute(hstmt);

		// SQLExecute Failed
		if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO)
		{
			std::wcerr << L"Database::GetCharList : Failed to Execute\n";
			ReleaseHandle(hstmt);
			return nullptr;
		}

		std::vector<int> charCodes;
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

		ReleaseObject(pCharList);

		return  m_JsonMaker.ToJsonString(*pCharList);
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

	// 지연쓰기를 써보고 싶긴 한데 지연쓰기는 별도의 서버를 하나 구성해야겠다.. (Paging Dirty bit느낌으로 하면 될듯?)
	// 일단 바로바로 반영하는 것으로 작성.
	// 연결을 끊을 때마다 기록하는것도 고려해볼수 있을지도..? (서버가 죽으면 어떡하지..?)
	bool UpdateCharInfo(CharInfo* pInfo_)
	{
		if (pInfo_ == nullptr)
		{
			return false;
		}

		HANDLE hstmt = m_Pool->Allocate();

		SQLRETURN retCode = SQLPrepare(hstmt, (SQLWCHAR*)
			L"UPDATE CHARINFO SET"
			L"LV = ?, EXPERIENCE = ?, STATPOINT = ?, HEALTHPOINT = ?, MANAPOINT = ?, "
			L"STRENGTH = ?, DEXTERITY = ?, INTELLIGENCE = ?, MENTALITY = ?, GOLD = ?, "
			L"LASTMAPCODE = ? "
			L"WHERE CHARNO = ?;", SQL_NTS);

		if (retCode != SQL_SUCCESS)
		{
			ReleaseHandle(hstmt);
			return false;
		}

		SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &pInfo_->Level, 0, nullptr);
		SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &pInfo_->Experience, 0, nullptr);
		SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &pInfo_->StatPoint, 0, nullptr);
		SQLBindParameter(hstmt, 4, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &pInfo_->HealthPoint, 0, nullptr);
		SQLBindParameter(hstmt, 5, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &pInfo_->ManaPoint, 0, nullptr);
		SQLBindParameter(hstmt, 6, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &pInfo_->Strength, 0, nullptr);
		SQLBindParameter(hstmt, 7, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &pInfo_->Dexterity, 0, nullptr);
		SQLBindParameter(hstmt, 8, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &pInfo_->Intelligence, 0, nullptr);
		SQLBindParameter(hstmt, 9, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &pInfo_->Mentality, 0, nullptr);
		SQLBindParameter(hstmt, 10, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &pInfo_->Gold, 0, nullptr);
		SQLBindParameter(hstmt, 11, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &pInfo_->LastMapCode, 0, nullptr);
		SQLBindParameter(hstmt, 12, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &pInfo_->CharNo, 0, nullptr);

		retCode = SQLExecute(hstmt);

		// SQLExecute Failed
		if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO)
		{
			std::cerr << "Database::GetCharInfo : Failed to Execute\n";
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

		SQLRETURN retCode = SQLExecute(hstmt);

		// SQLExecute Failed
		if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO)
		{
			ReleaseHandle(hstmt);
			std::cerr << "Database::MakePriceTable : Failed to Execute\n";
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
			MakeMonsterInfo(monstercode);
			ret.emplace_back(monstercode, maxhealth, pos);

			retCode = SQLFetch(hstmt);
		}

		ReleaseHandle(hstmt);

		return ret;
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
		SQLCloseCursor(stmt_);
		m_Pool->Deallocate(stmt_);

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

		SQLPrepare(hstmt, (SQLWCHAR*)
			L"SELECT FROM_MAP_CODE, TO_MAP_CODE "
			L"FROM MAP_EDGE", SQL_NTS);

		SQLRETURN retCode = SQLExecute(hstmt);

		// SQLExecute Failed
		if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO)
		{
			ReleaseHandle(hstmt);
			std::cerr << "Database::MakeMapEdgeList : Failed to Execute\n";
			return;
		}

		int fromcode;
		SQLLEN fromcodeLen;
		int tocode;
		SQLLEN tocodeLen;

		SQLBindCol(hstmt, 1, SQL_C_LONG, &fromcode, sizeof(fromcode), &fromcodeLen);
		SQLBindCol(hstmt, 2, SQL_C_LONG, &tocode, sizeof(tocode), &tocodeLen);

		retCode = SQLFetch(hstmt);

		while (retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO)
		{
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
			m_MapEdge.Insert(mapcode, npccode);

			retCode = SQLFetch(hstmt);
		}

		ReleaseHandle(hstmt);

		return;
	}

	void MakeMapMonsterInfo()
	{
		// T(MAPMONSTERINFO) : [MAPCODE][MONSTERCODE]

		HANDLE hstmt = m_Pool->Allocate();

		if (hstmt == INVALID_HANDLE_VALUE)
		{
			std::cerr << "DB::MakeMapMonsterInfo : 핸들 발급 실패\n";
			return;
		}

		SQLPrepare(hstmt, (SQLWCHAR*)
			L"SELECT MAPCODE, MONSTERCODE "
			L"FROM MAPMONSTERINFO", SQL_NTS);

		SQLRETURN retCode = SQLExecute(hstmt);

		// SQLExecute Failed
		if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO)
		{
			ReleaseHandle(hstmt);
			std::cerr << "Database::MakeMapMonsterInfo : Failed to Execute\n";
			return;
		}

		int mapcode;
		SQLLEN mapcodeLen;
		int monstercode;
		SQLLEN monstercodeLen;

		SQLBindCol(hstmt, 1, SQL_C_LONG, &mapcode, sizeof(mapcode), &mapcodeLen);
		SQLBindCol(hstmt, 2, SQL_C_LONG, &monstercode, sizeof(monstercode), &monstercodeLen);

		retCode = SQLFetch(hstmt);

		while (retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO)
		{
			m_MapEdge.Insert(mapcode, monstercode);

			retCode = SQLFetch(hstmt);
		}

		ReleaseHandle(hstmt);

		return;
	}

	// 사용가능성이 있는 몬스터에 대한 정보를 가져와 기록한다.
	void MakeMonsterInfo(const int monsterCode_)
	{
		// if(이미 있음) return;
		// 
		// DB조회
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

	std::vector<std::string> m_ReservedWord;
	std::unique_ptr<MSSQL::ConnectionPool> m_Pool;
	std::unique_ptr<MemoryPool<CharList>> m_CharListPool;
	std::unique_ptr<MemoryPool<CharInfo>> m_CharInfoPool;
};