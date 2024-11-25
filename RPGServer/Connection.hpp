#pragma once

#include "PacketData.hpp"
#include <queue>
#include <mutex>

class Connection
{
public:
	Connection(const SOCKET listenSocket_, const int index) : m_ListenSocket(listenSocket_), m_ClientIndex(index)
	{
		Init();
		BindAcceptEx();
	}

	virtual ~Connection()
	{
		std::lock_guard<std::mutex> guard(m);
		while (!m_SendQueue.empty())
		{
			delete m_SendQueue.front();
			m_SendQueue.pop();
		}
	}

	void Init()
	{
		ZeroMemory(mAcceptBuf, 64);
		ZeroMemory(mRecvBuf, MAX_SOCKBUF);
		ZeroMemory(&m_RecvOverlapped, sizeof(stOverlappedEx));

		m_RecvOverlapped.m_userIndex = m_ClientIndex;

		m_IsConnected.store(false);
		m_ClientSocket = INVALID_SOCKET;
	}

	void ResetConnection()
	{
		Init();
		BindAcceptEx();

		return;
	}

	bool BindIOCP(const HANDLE hWorkIOCP_)
	{
		auto hIOCP = CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_ClientSocket),
			hWorkIOCP_,
			(ULONG_PTR)(this),
			0);

		if (hIOCP == INVALID_HANDLE_VALUE || hIOCP != hWorkIOCP_)
		{
			return false;
		}

		m_IsConnected.store(true);

		return true;
	}

	bool BindRecv()
	{
		if (m_IsConnected.load() == false)
		{
			return false;
		}

		m_RecvOverlapped.m_eOperation = eIOOperation::RECV;
		m_RecvOverlapped.m_wsaBuf.len = MAX_SOCKBUF;
		m_RecvOverlapped.m_wsaBuf.buf = mRecvBuf;

		ZeroMemory(&m_RecvOverlapped.m_overlapped, sizeof(WSAOVERLAPPED));

		DWORD flag = 0;
		DWORD bytes = 0;

		auto result = WSARecv(
			m_ClientSocket,
			&m_RecvOverlapped.m_wsaBuf,
			1,
			&bytes,
			&flag,
			&m_RecvOverlapped.m_overlapped,
			NULL
		);

		if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
		{
			return false;
		}

		return true;
	}

	void SendMsg(PacketData* pData_)
	{
		if (m_IsConnected.load() == false)
		{
			delete pData_;
			return;
		}

		pData_->SetOverlapped();

		std::lock_guard<std::mutex> guard(m);

		m_SendQueue.push(pData_);

		if (m_SendQueue.size() == 1)
		{
			SendIO();
		}

		return;
	}

	bool SendIO()
	{
		PacketData* packet = m_SendQueue.front();

		DWORD dwRecvNumBytes = 0;

		auto result = WSASend(m_ClientSocket,
			&(packet->GetOverlapped().m_wsaBuf),
			1,
			&dwRecvNumBytes,
			0,
			(LPWSAOVERLAPPED) & (packet->GetOverlapped()),
			NULL);

		if (result == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING)
		{
			return false;
		}

		return true;
	}

	PacketData* SendCompleted()
	{
		std::lock_guard<std::mutex> guard(m);

		PacketData* ret = m_SendQueue.front();
		m_SendQueue.pop();

		// 전송처리하던 스레드가 이어서 전달하여 문맥교환 비용을 줄인다.
		if (!m_SendQueue.empty())
		{
			SendIO();
		}

		ret->Clear();

		return ret;
	}

	void Close(bool bIsForce_ = false)
	{
		m_IsConnected = false;

		struct linger stLinger = { 0,0 };

		if (bIsForce_)
		{
			stLinger.l_onoff = 1;
		}

		shutdown(m_ClientSocket, SD_BOTH);
		setsockopt(m_ClientSocket, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));
		closesocket(m_ClientSocket);
		m_ClientSocket = INVALID_SOCKET;

		return;
	}

	bool GetIP(uint32_t& out_)
	{
		struct sockaddr_in clientAddr;
		int addrLen = sizeof(clientAddr);

		if (m_ClientSocket == INVALID_SOCKET)
		{
			std::cerr << "Connection::GetIP : invalid socket.\n";
			return false;
		}

		if (getpeername(m_ClientSocket, reinterpret_cast<struct sockaddr*>(&clientAddr), &addrLen) == SOCKET_ERROR)
		{
			std::cerr << "Connection::GetIP : getpeername Failed " << WSAGetLastError() << '\n';
			return false;
		}

		out_ = clientAddr.sin_addr.S_un.S_addr;

		return true;
	}

	char* RecvBuffer() { return mRecvBuf; }
	unsigned short GetIndex() { return m_ClientIndex; }

private:
	bool BindAcceptEx()
	{
		ZeroMemory(&m_RecvOverlapped.m_overlapped, sizeof(WSAOVERLAPPED));

		m_ClientSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);

		if (m_ClientSocket == INVALID_SOCKET)
		{
			return false;
		}

		DWORD bytes = 0;
		DWORD flags = 0;
		m_RecvOverlapped.m_wsaBuf.len = 0;
		m_RecvOverlapped.m_wsaBuf.buf = nullptr;
		m_RecvOverlapped.m_eOperation = eIOOperation::ACCEPT;

		auto result = AcceptEx(
			m_ListenSocket,
			m_ClientSocket,
			mAcceptBuf,
			0,
			sizeof(SOCKADDR_IN) + 16,
			sizeof(SOCKADDR_IN) + 16,
			&bytes,
			reinterpret_cast<LPOVERLAPPED>(&m_RecvOverlapped)
		);

		if (result == FALSE && WSAGetLastError() != WSA_IO_PENDING)
		{
			return false;
		}

		return true;
	}

	SOCKET m_ListenSocket;
	SOCKET m_ClientSocket;
	stOverlappedEx m_RecvOverlapped;

	std::atomic<bool> m_IsConnected;
	unsigned short m_ClientIndex;

	std::mutex m;
	std::queue<PacketData*> m_SendQueue;

	char mAcceptBuf[64];
	char mRecvBuf[MAX_SOCKBUF];
};