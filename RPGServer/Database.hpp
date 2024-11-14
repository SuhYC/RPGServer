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
#include "Shop.hpp"
#include "MapEdge.hpp"

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

	// CharList Class �������� + �޸�Ǯ�� -> ��Ŷ�����Ͷ� ���ø����� ������?
	// CharNo �������� ��� �´�.
	// CharList�� Redis�� ���� �ʴ´�. (���� ��û�� ���Ӵ� 1������. ���� ������ ������ ĳ���ͼ����� ��û�ϴ����� Ȯ���ϰ� �������ұ�?)
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
		return m_Shop.GetPrice(itemcode_);
	}

	bool FindSalesList(const int npcCode_, const int itemCode_)
	{
		return m_Shop.FindSalesList(npcCode_, itemCode_);
	}

	bool FindMapEdge(const int fromMapCode_, const int toMapCode_)
	{
		return m_MapEdge.Find(fromMapCode_, toMapCode_);
	}

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
		SQLCloseCursor(stmt_);
		m_Pool->Deallocate(stmt_);

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
			m_Shop.InsertPrice(itemcode, purchasePrice);

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
			m_Shop.InsertSalesList(npccode, itemcode);

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

	RedisManager m_RedisManager;
	JsonMaker m_JsonMaker;
	Shop m_Shop;
	MapEdge m_MapEdge;

	std::vector<std::string> m_ReservedWord;
	std::unique_ptr<MSSQL::ConnectionPool> m_Pool;
	std::unique_ptr<MemoryPool<CharList>> m_CharListPool;
	std::unique_ptr<MemoryPool<CharInfo>> m_CharInfoPool;
};