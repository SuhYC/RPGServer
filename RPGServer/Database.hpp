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

	// usercode:int�� ����.
	int SignIn(std::wstring& id_, std::wstring& pw_)
	{

	}

	void LogOut(const int usercode_)
	{

	}
	
	// CharInfo Class �������� + �޸�Ǯ�� -> ��Ŷ�����Ͷ� ���ø����� ������?
	// Redis ���� ��ȸ�ϰ� ������ MSSQL ��ȸ
	// �ش� No�� �´� ĳ������ ������ ��� �´�.
	CharInfo* GetCharInfo()
	{

	}

	// CharList Class �������� + �޸�Ǯ�� -> ��Ŷ�����Ͷ� ���ø����� ������?
	// Redis ���� ��ȸ�ϰ� ������ MSSQL ��ȸ
	// CharNo �������� ��� �´�.
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
};