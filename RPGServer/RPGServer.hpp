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
#include <stdexcept>

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
		auto SendMsgFunc = [this](unsigned short connectionIndex, PacketData* pPacket) -> bool {return SendMsg(connectionIndex, pPacket); };

		m_ReqHandler.SendMsgFunc = SendMsgFunc;

		m_ReqHandler.Init(m_MaxClient);

		StartServer(m_MaxClient);
	}

	void End()
	{
		EndServer();
		m_ReqHandler.ClearSession();
	}

private:
	void OnConnect(const unsigned short index_, const uint32_t ip_) override
	{
		std::cout << "RPGServer::OnConnect : [" << index_ << "] Connected.\n";
		m_ReqHandler.SetIP(index_, ip_);
		return;
	}

	void OnReceive(const unsigned short index_, char* pData_, const DWORD ioSize_) override
	{
		// �����۷� ������ �ٲ۴ٰ� �ϸ� ���� �����͸� ��Ŷ ũ�⿡ �°� ����Ͽ� ReqHandler�� ��û�ؾ��Ѵ�.
		// ��Ŷ�� ũ�⺸�� ���� �����Ͱ� ���ٸ� ������ ��ҵ� �ʿ��ϴ�. (�Ƹ��� Connection�� �����ϴ°� ������?)
		
		//std::cout << "RPGServer::OnReceive : RecvMsg : " << pData_ << "\n";

		StoreMsg(index_, pData_, ioSize_);

		char buf[MAX_SOCKBUF];

		while (true)
		{

			//std::string req = GetMsg(index_);

			ZeroMemory(buf, MAX_SOCKBUF);
			unsigned int len = GetMsg(index_, buf);

			if (len == 0)
			{
				break;
			}

			std::string req(buf);

			if (!m_ReqHandler.HandleReq(index_, req))
			{
				std::cerr << "RPGServer::OnReceive : Failed to HandleReq\n";
				return;
			}
			else
			{
				//std::cout << "RPGServer::OnReceive : " << req << '\n';
			}
		}

		return;
	}

	void OnDisconnect(const unsigned short index_) override
	{
		std::cout << "RPGServer::OnDisconnect : [" << index_ << "] disconnected.\n";
		m_ReqHandler.HandleLogOut(index_);

		return;
	}
	/*
	bool CheckStringSize(std::string& str_, std::string& out_)
	{
		if (str_[0] != '[')
		{
			std::cerr << "RPGServer::CheckStringSize : Not Started with [\n";
			std::cerr << "RPGServer::CheckStringSize : string : " << str_ << '\n';
			return false;
		}

		size_t pos = str_.find(']');

		if (pos == std::string::npos)
		{
			return false;
		}

		try
		{
			std::string header = str_.substr(1, pos);
			int size = std::stoi(header);
			if (str_.size() > pos + size)
			{
				out_ = str_.substr(pos + 1, size);
				str_ = str_.substr(pos + size + 1);

				return true;
			}
		}
		catch (std::invalid_argument& e)
		{
			std::cerr << "RPGServer::CheckStringSize : " << e.what();
		}
		catch (...)
		{
			std::cerr << "RPGServer::CheckStringSize : Some Err\n";
		}

		return false;
	}*/

	const int m_BindPort;
	const unsigned short m_MaxClient;
	ReqHandler m_ReqHandler;
};