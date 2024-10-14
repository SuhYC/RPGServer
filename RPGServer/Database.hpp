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
*/

#include "ODBCConnectionPool.hpp"
#include "RedisManager.hpp"
#include "CharList.hpp"

class Database
{
public:
	Database()
	{
		m_Pool = std::make_unique<MSSQL::ConnectionPool>();
		MakeReservedWordList();
	}

	virtual ~Database()
	{
		m_Pool.reset();
	}

	bool SignUp(std::wstring& id_, std::wstring& pw_)
	{

	}

	// usercode:int로 리턴.
	int SignIn(std::wstring& id_, std::wstring& pw_)
	{

	}

	void LogOut(const int usercode_)
	{

	}
	
	// CharInfo Class 만들어야함 + 메모리풀도 -> 패킷데이터랑 템플릿으로 묶을까?
	// Redis 먼저 조회하고 없으면 MSSQL 조회
	// 해당 No에 맞는 캐릭터의 정보를 담아 온다.
	CharInfo* GetCharInfo()
	{

	}

	// CharList Class 만들어야함 + 메모리풀도 -> 패킷데이터랑 템플릿으로 묶을까?
	// Redis 먼저 조회하고 없으면 MSSQL 조회
	// CharNo 여러개를 담아 온다.
	CharList* GetCharList()
	{

	}

private:
	bool CheckIDIsValid()
	{

	}

	bool CheckPWIsValid()
	{

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
};