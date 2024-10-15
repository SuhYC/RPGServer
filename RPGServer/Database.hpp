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

const bool DB_DEBUG = true;
const int CLIENT_NOT_CERTIFIED = 0;
const int ALREADY_HAVE_SESSION = -1;
const int CHARINFO_MEMORY_POOL_COUNT = 100;
const int CHARLIST_MEMORY_POOL_COUNT = 100;

enum class eReturnCode
{
	SIGNUP_SUCCESS,
	SIGNUP_ALREADY_IN_USE,
	SIGNUP_FAIL
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
	}

	virtual ~Database()
	{
		m_Pool.reset();
		m_CharInfoPool.reset();
		m_CharListPool.reset();
	}

	eReturnCode SignUp(std::wstring& id_, std::wstring& pw_)
	{
		if (!CheckPWIsValid(pw_) || !CheckIDIsValid(id_))
		{
			// ��ȿ���� ���� �Է�
			return eReturnCode::SIGNUP_FAIL;
		}

		SQLWCHAR query[1024] = { 0 };
		sprintf_s((char*)query, 1024,
			"IF NOT EXISTS (SELECT 1 FROM LOGINDATA WHERE ID = '%s') "
			"BEGIN "
			"    INSERT INTO LOGINDATA (USERID, PASSWORD) VALUES ('%s', '%s'); "
			"    SELECT 'S' AS Result; "
			"END "
			"ELSE "
			"BEGIN "
			"    SELECT 'F' AS Result; "
			"END", id_.c_str(), id_.c_str(), pw_.c_str());

		HANDLE hstmt = m_Pool->Allocate();

		if (hstmt == INVALID_HANDLE_VALUE)
		{
			std::cerr << "Database::SignUp : �����ڵ� �߱޽���\n";
			return eReturnCode::SIGNUP_FAIL;
		}

		SQLPrepare(hstmt, query, SQL_NTS);

		SQLRETURN retCode = SQLExecute(hstmt);

		if (retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO)
		{
			SQLLEN num_rows;
			SQLCHAR result_message[1024];
			retCode = SQLFetch(hstmt);
			if (retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO)
			{
				SQLGetData(hstmt, 1, SQL_C_CHAR, result_message, sizeof(result_message), &num_rows);
				if (DB_DEBUG)
				{
					std::cout << result_message << "\n";
				}

				if (!strcmp((const char*)result_message, "S"))
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
			// SQLFetch Failed
			else
			{
				ReleaseHandle(hstmt);
				std::cerr << "Database::SIGNUP : Failed to Fetch\n";
				return eReturnCode::SIGNUP_FAIL;
			}
		}
		// SQLExcute Failed
		else
		{
			ReleaseHandle(hstmt);
			std::cerr << "Database::SIGNUP : Failed to Execute\n";
			return eReturnCode::SIGNUP_FAIL;
		}

	}

	// usercode:int�� ����.
	// 0 : Client Not Certified.
	// -1 : Already Have Session
	int SignIn(std::wstring& id_, std::wstring& pw_, int ip_)
	{
		if (!CheckIDIsValid(id_) || !CheckPWIsValid(pw_))
		{
			return CLIENT_NOT_CERTIFIED;
		}

		SQLWCHAR query[1024] = { 0 };

		sprintf_s((char*)query, 1024,
			"IF EXISTS (SELECT 1 FROM LOGINDATA WHERE USERID = '%s' AND PASSWORD = '%s') "
			"BEGIN "
			"    SELECT USERCODE WHERE ID = '%s' AS Result; "
			"END "
			"ELSE "
			"BEGIN "
			"    SELECT 0 AS Result; "
			"END", id_.c_str(), pw_.c_str(), id_.c_str());

		HANDLE hstmt = m_Pool->Allocate();
		
		if (hstmt == INVALID_HANDLE_VALUE)
		{
			std::cerr << "Database::SignIn : �����ڵ� �߱� ����\n";
			return CLIENT_NOT_CERTIFIED;
		}

		SQLPrepare(hstmt, query, SQL_NTS);

		SQLRETURN retCode = SQLExecute(hstmt);

		if (retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO)
		{
			SQLLEN num_rows;
			long result = 0;
			retCode = SQLFetch(hstmt);
			if (retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO)
			{
				SQLGetData(hstmt, 1, SQL_C_LONG, &result, sizeof(long), &num_rows);
				if (DB_DEBUG)
				{
					std::cout << "SIGNIN RESULT : " << result << "\n";
				}

				// CLIENT_NOT_CERTIFIED -> Failed to UPDATE : INCORRECT OR ALREADY LOGGED IN
				if (result == CLIENT_NOT_CERTIFIED)
				{
					ReleaseHandle(hstmt);
					return CLIENT_NOT_CERTIFIED;
				}
				// USERCODE -> SUCCESS to Signin
				else
				{
					ReleaseHandle(hstmt);

					if (TrySignIn(result, ip_))
					{
						return result;
					}
					return ALREADY_HAVE_SESSION; // other user using this id.
				}
			}
			// SQLFetch Failed
			else
			{
				std::cerr << "DB::SIGNUP : Failed to Fetch\n";
				ReleaseHandle(hstmt);
				return CLIENT_NOT_CERTIFIED;
			}
		}
		// SQLExcute Failed
		else
		{
			std::cerr << "DB::SIGNUP : Failed to Execute\n";
			ReleaseHandle(hstmt);
			return CLIENT_NOT_CERTIFIED;
		}
	}

	void LogOut(const int userCode_)
	{
		m_RedisManager.LogOut(userCode_);
	}
	
	// CharInfo Class �������� + �޸�Ǯ�� -> ��Ŷ�����Ͷ� ���ø����� ������?
	// Redis ���� ��ȸ�ϰ� ������ MSSQL ��ȸ
	// �ش� No�� �´� ĳ������ ������ ��� �´�.
	CharInfo* GetCharInfo(const int charNo_)
	{
		CharInfo* ret = m_CharInfoPool->Allocate();

		if (m_RedisManager.GetCharInfo(charNo_, ret))
		{
			return ret;
		}

		SQLWCHAR query[1024] = { 0 };

		// ���� �ۼ� �� ���� ����� ���� ret init �� ����.
	}

	// CharList Class �������� + �޸�Ǯ�� -> ��Ŷ�����Ͷ� ���ø����� ������?
	// CharNo �������� ��� �´�.
	CharList* GetCharList(const int userCode_)
	{

	}

	void ReleaseObject(CharInfo* pObj_)
	{
		m_CharInfoPool->Deallocate(pObj_);

		return;
	}

	void ReleaseObject(CharList* pObj_)
	{
		m_CharListPool->Deallocate(pObj_);

		return;
	}

private:
	bool CheckIDIsValid(std::wstring& id_)
	{

	}

	bool CheckPWIsValid(std::wstring& pw_)
	{

	}

	bool TrySignIn(const int usercode_, const int ip_)
	{
		return m_RedisManager.MakeSession(usercode_, ip_);
	}

	void ReleaseHandle(HANDLE stmt_)
	{
		SQLCloseCursor(stmt_);
		m_Pool->Deallocate(stmt_);

		return;
	}

	bool InjectionCheck(const std::wstring& str_)
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

		for (const std::wstring& word : m_ReservedWord)
		{
			if (IsStringContains(str_, word))
			{
				return false;
			}
		}

		return true;
	}

	// Todo. RabinKarf�� ��ü����.
	bool IsStringContains(const std::wstring& orgstr_, const std::wstring& somestr_)
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
		m_ReservedWord.push_back(L"OR");
		m_ReservedWord.push_back(L"SELECT");
		m_ReservedWord.push_back(L"INSERT");
		m_ReservedWord.push_back(L"DELETE");
		m_ReservedWord.push_back(L"UPDATE");
		m_ReservedWord.push_back(L"CREATE");
		m_ReservedWord.push_back(L"DROP");
		m_ReservedWord.push_back(L"EXEC");
		m_ReservedWord.push_back(L"UNION");
		m_ReservedWord.push_back(L"FETCH");
		m_ReservedWord.push_back(L"DECLARE");
		m_ReservedWord.push_back(L"TRUNCATE");

		return;
	}

	RedisManager m_RedisManager;
	std::vector<std::wstring> m_ReservedWord;
	std::unique_ptr<MSSQL::ConnectionPool> m_Pool;
	std::unique_ptr<MemoryPool<CharList>> m_CharListPool;
	std::unique_ptr<MemoryPool<CharInfo>> m_CharInfoPool;
};