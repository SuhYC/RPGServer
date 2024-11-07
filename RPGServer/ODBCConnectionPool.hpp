#pragma once

#include "Define.hpp"

#include <Windows.h>
#include <sqlext.h>
#include <string>
#include <vector>

namespace MSSQL
{
	const int OBJECT_COUNT = 3;

	class ConnectionPool
	{
	public:
		ConnectionPool()
		{
			SetEnv();
			for (int i = 0; i < OBJECT_COUNT; i++)
			{
				if (!MakeNewConnection())
				{
					std::cout << "ODBCConnectionPool::MakeNewConnection : 커넥션 생성 실패\n";
				}
			}
			std::cout << "odbc 준비 완료...\n";
		}

		~ConnectionPool()
		{
			int cnt = 0;
			HANDLE tmp;
			while (q.try_pop(tmp))
			{
				cnt++;
				SQLCancel(tmp);
				SQLFreeHandle(SQL_HANDLE_STMT, tmp);
			}

			std::cout << "hstmt:" << cnt << "\n";

			for (int i = 0; i < OBJECT_COUNT; i++)
			{
				SQLFreeHandle(SQL_HANDLE_DBC, HDBCs[i]);
			}

			SQLFreeHandle(SQL_HANDLE_ENV, henv);
		}

		void Deallocate(HANDLE hstmt_)
		{
			SQLCloseCursor(hstmt_);
			q.push(hstmt_);
		}

		HANDLE Allocate()
		{
			HANDLE ret = INVALID_HANDLE_VALUE;

			if (q.try_pop(ret))
			{
				return ret;
			}

			std::cerr << "메모리 풀 부족\n";

			return INVALID_HANDLE_VALUE;
		}

	private:
		bool SetEnv()
		{
			SQLRETURN retcode;

			retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

			if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
			{
				return false;
			}

			retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

			if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
			{
				return false;
			}

			return true;
		}

		bool MakeNewConnection()
		{
			SQLRETURN retcode;
			HANDLE hdbc = INVALID_HANDLE_VALUE;
			HANDLE hstmt = INVALID_HANDLE_VALUE;

			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
			{
				return false;
			}

			std::wstring odbc = L"RPGDB";
			std::wstring id = L"KIM";
			std::wstring pwd = L"kim123";

			SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (void*)5, 0);

			retcode = SQLConnect(hdbc, (SQLWCHAR*)odbc.c_str(), SQL_NTS, (SQLWCHAR*)id.c_str(), SQL_NTS, (SQLWCHAR*)pwd.c_str(), SQL_NTS);

			if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
			{
				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
				return false;
			}

			SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

			if (hstmt == INVALID_HANDLE_VALUE)
			{
				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
				return false;
			}

			q.push(hstmt);
			HDBCs.push_back(hdbc);
		}

		Concurrency::concurrent_queue<HANDLE> q;
		HANDLE henv;
		std::vector<HANDLE> HDBCs;
	};

}
