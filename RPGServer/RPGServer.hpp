#pragma once

// ���߼����� ���� ������ MSSQL���� Redis�� �ѱ���.
// ������ƮǮ, Ŀ�ؼ�Ǯ�� �ִ��� Ȱ��

/*
* ������ �� �� �ִ� ����
* 1. �κ�
* �α���
* ȸ������
* ��й�ȣ ����
* ĳ���� ��� ��������
* ĳ���� ���� ��������
* ĳ���� �����ϱ� (�������� ���۰� �����ϴ� ������ �и��Ѵ�.)
* 
* 2. ���Ӿ�
* ��ų���� -> ��ų�ڵ� + �ǰ��� ���� ���(can be null)
* ������ �ݱ� -> �ݱ� �õ��� �������� �ʻ� ������Ʈ �ε���
* ������ ���� -> �ŷ��� ���� + ������ ������ �ڵ�
* ������ ������ -> �������� �������� �κ��丮�ε���
* 
* ������ ó���ؾ��ϴ� ����
* �α��� -> DB�˻� -> true : Redis�� ���ε� -> UserManager �ݿ� -> ��� ����
*				   -> false : ��� ����
* ȸ������ -> DB�ݿ� -> ��� ����
* 
* ��й�ȣ���� -> DB�ݿ� -> ��� ����
* 
* ĳ���� ��� �������� -> DBȮ��(Redis�� ����.) -> ��� ���� (�� ������ Ŭ���̾�Ʈ�� ������ ���� ��.)
* 
* ĳ���� ���� �������� -> DBȮ��(Redis�� ���� ���� �� �ִ�.) -> ��� ���� 
* 
* ĳ���� �����ϱ� -> Redis Ȯ�� (Ŭ���̾�Ʈ������ �����������⸦ �õ��ϰ� �����ϱ⸦ ����.) -> UserManager �ݿ�
* 
* ��ų���� -> User ������ �´� ������ ��� -> ���� �ݿ� -> Map�� �ִ� �������� ����
*													kill -> ������ ��� -> Map�� �������� ����
* 
* ������ �ݱ� -> Map���� �ش� ������ Ȯ�� �� ���� ->		�ش� ���� : ��� ����
*												  -> true : �ٸ� ���� : �ش� ������ ȹ���� ���� (usercode, itemidx)
*												  -> false : -
* 
* ������ ���� -> DBȮ�� (���������� ���� Redis�� ���� �� �ִ�.) -> Redis�� �л�� �ɰ� ������ ���� �õ� -> ��� ���� // ���𽺴� Ʈ����� ������ ���� ���� �ɰ� �����ؾ���.
* 
* ������ ������ -> Redis���� �л�� �ɰ� �κ��丮�� �����Ѵ� -> Map�� ������ ������Ʈ�� �����Ѵ�. -> ��� �������� �����Ѵ�.
*/

#include "IOCP.hpp"
#include "Database.hpp"
#include "UserManager.hpp"
#include <Windows.h>
#include "MapManager.hpp"

class RPGServer : IOCPServer
{
public:
	RPGServer(const int nBindPort_, const unsigned short MaxClient_) : m_BindPort(nBindPort_), m_MaxClient(MaxClient_)
	{
		InitSocket(nBindPort_);
	}

	~RPGServer()
	{

	}

	void Start()
	{
		m_UserManager.Init(m_MaxClient);

		StartServer(m_MaxClient);
	}

	void End()
	{
		m_UserManager.Clear();

		EndServer();
	}

private:
	void OnConnect(const unsigned short index_) override
	{

		return;
	}

	void OnReceive(const unsigned short index_, char* pData_, const DWORD ioSize_) override
	{
		std::string str;
		str.assign(pData_, ioSize_);
		PacketData* pPacket = MakePacket(index_, str);

		SendMsg(pPacket);

		return;
	}

	void OnDisconnect(const unsigned short index_) override
	{
		User* pUser = m_UserManager.GetUserByConnIndex(index_);
		int usercode = pUser->GetUserCode();

		if (usercode != CLIENT_NOT_CERTIFIED)
		{
			m_DB.LogOut(usercode);
		}

		m_UserManager.ReleaseUser(index_);

		return;
	}

	PacketData* MakePacket(const unsigned short index_, std::string& str_)
	{
		char* msg = new char[str_.size() + 1];
		CopyMemory(msg, str_.c_str(), str_.size());
		msg[str_.size()] = NULL;

		PacketData* pPacket = AllocatePacket();

		if (pPacket == nullptr)
		{
			return nullptr;
		}

		pPacket->Init(index_, msg, str_.size());

		return pPacket;
	}

	const int m_BindPort;
	const unsigned short m_MaxClient;
	Database m_DB;
	UserManager m_UserManager;
	MapManager m_MapManager;
};