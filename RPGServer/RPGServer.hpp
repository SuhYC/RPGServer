#pragma once

// 다중서버에 대한 세션을 MSSQL에서 Redis로 넘기자.
// 메모리풀, 커넥션풀을 최대한 활용
// 

#include "IOCP.hpp"
#include "Database.hpp"

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
		StartServer(m_MaxClient);
	}

	void End()
	{
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
};