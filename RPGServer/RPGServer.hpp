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
#include "UserManager.hpp"
#include <Windows.h>
#include "MapManager.hpp"
#include "ReqHandler.hpp"
#include <functional>

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
		auto AlloFunc = [this]() -> PacketData* { return AllocatePacket(); };
		auto DealloFunc = [this](PacketData* pPacket) { DeallocatePacket(pPacket); };
		auto SendMsgFunc = [this](PacketData* pPacket) -> bool {return SendMsg(pPacket); };

		m_ReqHandler.AllocatePacket = AlloFunc;
		m_ReqHandler.DeallocatePacket = DealloFunc;
		m_ReqHandler.SendMsgFunc = SendMsgFunc;

		m_ReqHandler.Init(m_MaxClient);

		StartServer(m_MaxClient);
	}

	void End()
	{
		EndServer();
	}

private:
	void OnConnect(const unsigned short index_, std::string& ip_) override
	{
		m_ReqHandler.SetIP(index_, ip_);
		return;
	}

	void OnReceive(const unsigned short index_, char* pData_, const DWORD ioSize_) override
	{
		std::string str;
		str.assign(pData_, ioSize_);

		m_ReqHandler.HandleReq(index_, str);

		return;
	}

	void OnDisconnect(const unsigned short index_) override
	{
		m_ReqHandler.HandleLogOut(index_);

		return;
	}

	const int m_BindPort;
	const unsigned short m_MaxClient;
	ReqHandler m_ReqHandler;
};