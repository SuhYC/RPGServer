#pragma once

/*
* �ش� ���Ͽ��� MSSQL, Redis�� ���õ� ����� �����Ѵ�.
* Redis���� ����� RedisManager���� hiredis���� ����� ����Ѵ�.
* 
* �����ڵ��� ������ ����Ѵ� �ϴ��� �ϳ��� hdbc�� �����ϴ°� thread-safe���� ���� �� �ִٴ� �ǰ�.
* -> ������ �����ڵ鸶�� ȯ��,�����ڵ��� ���� ��������.
* 
* signin -> userno ȹ��
* GetCharList(userno) -> charno + ... ȹ�� TABLE(CHARLIST)
* GetCharInfo(charno) -> 
* 
* CharList, CharInfo �ν��Ͻ��� ��� �� ����� �ݳ��� �� �ְ� �Լ��� ������.
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
	SYSTEM_ERROR // �ý��� ����
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
			// ��ȿ���� ���� �Է�
			return eReturnCode::SIGNUP_FAIL;
		}

		HANDLE hstmt = m_Pool->Allocate();

		if (hstmt == INVALID_HANDLE_VALUE)
		{
			std::cerr << "Database::SignUp : �����ڵ� �߱޽���\n";
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

	// usercode:int�� ����.
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
			std::cerr << "Database::SignIn : �����ڵ� �߱� ����\n";
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
	
	// CharInfo Class �������� + �޸�Ǯ�� -> ��Ŷ�����Ͷ� ���ø����� ������?
	// Redis ���� ��ȸ�ϰ� ������ MSSQL ��ȸ
	// �ش� No�� �´� ĳ������ ������ ��� �´�.
	std::string GetCharInfoJsonStr(const int charNo_)
	{
		std::string ret;
		// CacheServer ���� Ȯ��
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

		// ���𽺿� ����ϱ�
		if (m_RedisManager.CreateCharInfo(charNo_, ret) == false)
		{
			std::wcerr << L"Database::GetCharInfoJsonStr : ���𽺿� ���� ��� ����\n";
		}

		return ret;
	}

	// CharInfo Class �������� + �޸�Ǯ�� -> ��Ŷ�����Ͷ� ���ø����� ������?
	// Redis ���� ��ȸ�ϰ� ������ MSSQL ��ȸ
	// �ش� No�� �´� ĳ������ ������ ��� �´�.
	// �ݵ�� return���� �޾ư� CharInfo ��ü�� ReleaseObject�� ��ȯ�ؾ��Ѵ�.
	CharInfo* GetCharInfo(const int charNo_)
	{
		CharInfo* ret = m_CharInfoPool->Allocate();

		if (ret == nullptr)
		{
			std::cerr << "DB::GetCharInfo : Failed to Allocate Info Instance\n";
			return nullptr;
		}

		std::string strCharInfo;

		// CacheServer ���� Ȯ��
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
			std::cerr << "Database::GetCharInfo : Json����ȭ ����.\n";
			return ret;
		}

		// ���𽺿� ����ϱ�
		if (m_RedisManager.CreateCharInfo(charNo_, strCharInfo) == false)
		{
			std::wcerr << L"Database::GetCharInfo : ���𽺿� ���� ��� ����\n";
		}

		return ret;
	}

	// ToJsonException�� �߻��� �� ����.
	// CharList Class �������� + �޸�Ǯ�� -> ��Ŷ�����Ͷ� ���ø����� ������?
	// CharNo �������� ��� �´�.
	// CharList�� Redis�� ���� �ʴ´�. (���� ��û�� ���Ӵ� 1������. ���� ������ ������ ĳ���ͼ����� ��û�ϴ����� Ȯ���ϰ� �������ұ�?)
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

	// ���Ű��ݰ� ������ �����ݸ� �Ǵ��Ѵ�.
	// ������ ��ġ, ���ѵ��� �������� �ʴ´�.
	bool BuyItem(const int charCode_, const int itemCode_, const time_t extime_, const int count_)
	{
		int price = GetPrice(itemCode_);

		// �ش��ϴ� ������ ���� ����
		if (price == -1)
		{
			return false;
		}

		REDISRETURN eRet;

		// �� ȹ�� ���� �� �絵��
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

	// return -1 : �ش��ϴ� ������ ���� ����.
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

	// ������ �κ��丮�� �������� �Ѹ��� ���.
	// �ѷ����� �������� �ڵ常 �����Ѵ�.
	// return 0 : ������, return n : �ڵ尡 n�� �������� ����ϴµ� �����ߴ�.
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

	// �����ڵ忡 �´� ��������� ��� �����´�.
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

	// �������⸦ �Ẹ�� �ͱ� �ѵ� ��������� ������ ������ �ϳ� �����ؾ߰ڴ�.. (Paging Dirty bit�������� �ϸ� �ɵ�?)
	// �ϴ� �ٷιٷ� �ݿ��ϴ� ������ �ۼ�.
	// ������ ���� ������ ����ϴ°͵� ����غ��� ��������..? (������ ������ �����..?)
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

	// GameMap�� �ʱ�ȭ���� ���ǹǷ� DB�� �ٷ� ��ȸ�ص� �ɰͰ���.
	// ���ο��� ���� ��������� �ѹ��� �������� ������
	std::vector<MonsterSpawnInfo> GetMonsterSpawnInfo(const int mapcode_)
	{
		// T(SPAWNINFO) [MAPCODE] [MONSTERCODE] [POSX] [POSY]
		// T(MONSTERINFO)[MONSTERCODE][MAXHEALTH][EXPERIENCE][DROPGOLD]
		// T(DROPINFO) [MONSTERCODE] [ITEMCODE] [RATIO]

		HANDLE hstmt = m_Pool->Allocate();

		if (hstmt == INVALID_HANDLE_VALUE)
		{
			std::cerr << "DB::GetMonsterSpawnInfo : �ڵ� �߱� ����.\n";
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

		// Redis�� �������� ����� ���� (�ٸ� ������ �̹� ������)
		if (bRet == false)
		{
			return false;
		}

		// ��ȿ�� �г����� �ƴ�.
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

		// �����ͺ��̽����� �ش� �г����� �˻���
		if (SQLFetch(hstmt) == SQL_SUCCESS)
		{
			ReleaseHandle(hstmt);
			m_RedisManager.CancelReserveNickname(nickname_);
			return false;
		}
		// �˻��� �г����� ����.
		else
		{
			ReleaseHandle(hstmt);
			return true;
		}
	}

	// ReserveNickname�� ������ �������� Ȯ���ϰ� ȣ���� ��.
	bool CancelReserveNickname(std::string& nickname_)
	{
		return m_RedisManager.CancelReserveNickname(nickname_);
	}

	// ������ �����ߴٸ� charno�� ����.
	// ������ �����ߴٸ� -1�� ����.
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

		// CacheServer ���� Ȯ��
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

		// ���𽺿� ����ϱ�
		if (!m_RedisManager.CreateInven(charcode_, out_))
		{
			std::wcerr << L"Database::GetInventory : ���𽺿� ���� ��� ����\n";
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

		// ���� ���� Ȯ��.
		if (m_RedisManager.GetSalesList(npcCode_, ret))
		{
			return ret;
		}

		auto pair = m_SalesList.Get(npcCode_);

		// �ش��ϴ� �� ����
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

		// ���� ��ҹ���, ���ڸ� ���
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

		// ���� ��ҹ���, ���ڸ� ���
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

	// '��' ���� ���ڴ� unicode������ �� ���ڰ� euc-kr������ �ѱ۷� �νĵ��� ���� �� �ִ�.
	bool isHangul(unsigned char byte1, unsigned char byte2) {
		// EUC-KR�� �ѱ� ������ ù ����Ʈ�� 0xA1 ~ 0xFE, �� ��° ����Ʈ�� 0x41 ~ 0x5A
		unsigned short combined = (byte1 << 8) | byte2;

		// U+AC00 ~ U+D7A3 ���� Ȯ��
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

			// 2����Ʈ ���� (EUC-KR���� �ѱ�)
			if (byte1 >= 0xA1 && byte1 <= 0xFE && i + 1 < nickname.length()) {
				unsigned char byte2 = static_cast<unsigned char>(nickname[i + 1]);

				// �ѱ����� Ȯ��
				if (!isHangul(byte1, byte2)) {
					return true;  // �ѱ��� �ƴ� 2����Ʈ ����
				}
				i += 2;  // �ѱ��� 2����Ʈ�̹Ƿ� 2ĭ �̵�
			}
			// 1����Ʈ ���� (����, ����, Ư������ ��)
			else {
				if (!(isAlpha(byte1) || isDigit(byte1))) {
					return true;  // ����/���ڰ� �ƴ� ���ڰ� ����
				}
				i++;  // 1����Ʈ ������ ��� 1ĭ �̵�
			}
		}
		return false;  // ��� ���ڰ� ���� ������ ���
	}

	// ������ SetNx�� �̿�, �ش� �����ڵ�� �α����� ������ ���ٸ� ����Ѵ�.
	// true : success to SignIn
	// false : fail to SignIn (Already In Use)
	bool TrySignIn(const int usercode_, const std::string& ip_)
	{
		return m_RedisManager.MakeSession(usercode_, ip_);
	}

	// ����� �����ڵ��� ���¸� �ʱ�ȭ�ϰ� ������ �� �ֵ��� Ŀ�ؼ�Ǯ�� �ִ´�.
	void ReleaseHandle(HANDLE stmt_)
	{
		//SQLCloseCursor(stmt_);
		SQLFreeStmt(stmt_, SQL_UNBIND); // BIND COL ����
		SQLFreeStmt(stmt_, SQL_CLOSE); // Ŀ�� �ݰ� ������ ��� ����
		SQLFreeStmt(stmt_, SQL_RESET_PARAMS); // BIND PARAM ����
		m_Pool->Deallocate(stmt_); // Ŀ�ؼ�Ǯ ��ȯ

		return;
	}

	bool InjectionCheck(const std::string& str_)
	{
		// Ư����ȣ ����
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

		// ����� ����

		for (const std::string& word : m_ReservedWord)
		{
			if (IsStringContains(str_, word))
			{
				return false;
			}
		}

		return true;
	}

	// Todo. RabinKarf�� ��ü����.
	bool IsStringContains(const std::string& orgstr_, const std::string& somestr_)
	{
		size_t strsize1 = orgstr_.size(), strsize2 = somestr_.size();

		for (size_t i = 0; i < strsize1; i++)
		{
			// �� �̻� ã�� �� ����
			if (i + strsize2 > strsize1)
			{
				return false;
			}

			if (orgstr_[i] == somestr_[0])
			{
				// ���� ���ڿ��ΰ�
				bool check = true;

				for (size_t j = 1; j < strsize2; j++)
				{
					// ���� ���ڿ��� �ƴ�
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

		// ����� : OR, SELECT, INSERT, DELETE, UPDATE, CREATE, DROP, EXEC, UNION, FETCH, DECLARE, TRUNCATE
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
		// MSSQL���� ITEMTABLE�� �ִ� ���� �ܾ����.
		// T(ITEMTABLE) : [ITEMCODE][PURCHASEPRICE]
		// �ϴ��� �������� ������ ������ ������ 10MB�� �Ѿ�� 10MB������ �߶� �������� ���� ���ٰ� �Ѵ�.
		// �ϴ��� �ະ �����ʹ� 8byte�� �����Ǿ� �ִ�.

		HANDLE hstmt = m_Pool->Allocate();

		if (hstmt == INVALID_HANDLE_VALUE)
		{
			std::cerr << "DB::MakePriceTable : �ڵ� �߱� ����\n";
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
			std::cerr << "DB::MakeSalesList : �ڵ� �߱� ����\n";
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
			std::cerr << "DB::MakeMapEdgeList : �ڵ� �߱� ����\n";
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
			std::cerr << "DB::MakeMapNPCInfo : �ڵ� �߱� ����\n";
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

	// ĳ���� ������ DB�� �����Ѵ�.
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

	// ������ ĳ������ CHARNO�� �޾ƿ´�.
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

	// USERNO�� CHARNO�� �����ϴ� ������ DB�� �߰��Ѵ�.
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

	// ��밡�ɼ��� �ִ� ���Ϳ� ���� ��������� ������ ����Ѵ�.
	// ���Ͱ� ���� �ƹ��͵� ������� �ʴ��� DB�� Gold�� ����� Ȯ���� ���´�. -> �ش� ���� ������ �ִ��� �Ǵ��ϴµ� �� ��.
	void MakeMonsterDropTable(const int monsterCode_)
	{
		// T(DROPINFO) [MONSTERCODE] [ITEMCODE] [NUM] [DEN]
		// NUM / DEN = ���Ȯ��

		auto range = m_DropInfo.Get(monsterCode_);
		// ���� ���� (begin() != end())
		if (range.first != range.second)
		{
			return;
		}

		HANDLE hstmt = m_Pool->Allocate();

		if (hstmt == INVALID_HANDLE_VALUE)
		{
			std::cerr << "DB::MakeMonsterDropTable : �ڵ� �߱� ����\n";
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

		// ���� �޽��� ��������
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
		// EUC-KR(CP949) -> UTF-16 ��ȯ�� ���� ���� ũ�� ���
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
	// ù��° �Ķ������ �����κ��� �ι�°�Ķ������ ������ �̵� �������� ǥ��.
	UMultiMapWrapper<int, int> m_MapEdge;

	// [mapcode][npccode]
	// �ش� �ʿ� �ش� npc�� �������� ǥ��.
	UMultiMapWrapper<int, int> m_MapNPCInfo;

	// [monstercode][DropInfo(itemcode, num, den)]
	// �ش� ���Ͱ� itemcode�� �������� num / den Ȯ���� ����߸��� ǥ��.
	UMultiMapWrapper<int, MonsterDropInfo> m_DropInfo;

	// [monstercode][Experience]
	// �ش� ���͸� �����߸��� ��� ����ġ�� ǥ��.
	UMapWrapper<int, int> m_MonsterEXPInfo;

	// [monstercode][Gold]
	// �ش� ���͸� �����߸��� ��� ��� ��带 ǥ��.
	UMapWrapper<int, int> m_MonsterGoldInfo;

	std::vector<std::string> m_ReservedWord;
	std::unique_ptr<MSSQL::ConnectionPool> m_Pool;
	std::unique_ptr<MemoryPool<CharList>> m_CharListPool;
	std::unique_ptr<MemoryPool<CharInfo>> m_CharInfoPool;
};