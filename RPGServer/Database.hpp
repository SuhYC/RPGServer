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
			// 유효하지 않은 입력
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
			std::cerr << "Database::SignUp : 구문핸들 발급실패\n";
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

	// usercode:int로 리턴.
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
			std::cerr << "Database::SignIn : 구문핸들 발급 실패\n";
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
	
	// CharInfo Class 만들어야함 + 메모리풀도 -> 패킷데이터랑 템플릿으로 묶을까?
	// Redis 먼저 조회하고 없으면 MSSQL 조회
	// 해당 No에 맞는 캐릭터의 정보를 담아 온다.
	CharInfo* GetCharInfo(const int charNo_)
	{
		CharInfo* ret = m_CharInfoPool->Allocate();

		if (m_RedisManager.GetCharInfo(charNo_, ret))
		{
			return ret;
		}

		SQLWCHAR query[1024] = { 0 };

		// 쿼리 작성 및 쿼리 결과에 따른 ret init 후 리턴.
	}

	// CharList Class 만들어야함 + 메모리풀도 -> 패킷데이터랑 템플릿으로 묶을까?
	// CharNo 여러개를 담아 온다.
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

		for (const std::wstring& word : m_ReservedWord)
		{
			if (IsStringContains(str_, word))
			{
				return false;
			}
		}

		return true;
	}

	// Todo. RabinKarf로 대체하자.
	bool IsStringContains(const std::wstring& orgstr_, const std::wstring& somestr_)
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