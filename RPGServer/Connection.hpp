#pragma once

#include "PacketData.hpp"
#include "RingBuffer.hpp"
#include <queue>
#include <mutex>

#define GET_ACCEPTEX_SOCKADDRS( lpOutputBuffer, dwBytesReceived, lpLocalAddress, lpRemoteAddress ) \
    do { \
        sockaddr* pLocal = (sockaddr*)lpLocalAddress; \
        sockaddr* pRemote = (sockaddr*)lpRemoteAddress; \
        int localSize = sizeof(sockaddr_in); \
        int remoteSize = sizeof(sockaddr_in); \
        memcpy(pLocal, lpOutputBuffer, localSize); \
        memcpy(pRemote, lpOutputBuffer + localSize, remoteSize); \
    } while (0)

const int MAX_RINGBUFFER_SIZE = 1000;

class Connection
{
public:
	Connection(const SOCKET listenSocket_, const int index) : m_ListenSocket(listenSocket_), m_ClientIndex(index)
	{
		Init();
		BindAcceptEx();
		m_SendBuffer.Init(MAX_RINGBUFFER_SIZE);
	}

	virtual ~Connection()
	{

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
		if (pData_ == nullptr)
		{
			return;
		}

		if (m_IsConnected.load() == false)
		{
			delete pData_;
			return;
		}

		std::lock_guard<std::mutex> guard(m_mutex);

		m_SendBuffer.enqueue(pData_->GetBlock()->pData, pData_->GetBlock()->dataSize);

		if (m_SendBuffer.Size() == pData_->GetBlock()->dataSize)
		{
			SendIO();
		}

		return;
	}

	bool SendIO()
	{
		int datasize = m_SendBuffer.dequeue(m_SendingBuffer, MAX_SOCKBUF);

		ZeroMemory(&m_SendOverlapped, sizeof(stOverlappedEx));

		m_SendOverlapped.m_wsaBuf.len = datasize;
		m_SendOverlapped.m_wsaBuf.buf = m_SendingBuffer;
		m_SendOverlapped.m_eOperation = eIOOperation::SEND;
		m_SendOverlapped.m_userIndex = m_ClientIndex;

		DWORD dwRecvNumBytes = 0;

		auto result = WSASend(m_ClientSocket,
			&(m_SendOverlapped.m_wsaBuf),
			1,
			&dwRecvNumBytes,
			0,
			(LPWSAOVERLAPPED) & (m_SendOverlapped),
			NULL);

		if (result == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING)
		{
			return false;
		}

		return true;
	}

	void SendCompleted()
	{
		std::lock_guard<std::mutex> guard(m_mutex);

		if (!m_SendBuffer.IsEmpty())
		{
			SendIO();
		}

		return;
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
		setsockopt(m_ClientSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&m_ListenSocket, (int)sizeof(SOCKET));

		SOCKADDR* l_addr = nullptr; SOCKADDR* r_addr = nullptr; int l_size = 0, r_size = 0;

		GetAcceptExSockaddrs(mAcceptBuf,
			0,
			sizeof(SOCKADDR_STORAGE) + 16,
			sizeof(SOCKADDR_STORAGE) + 16,
			&l_addr,
			&l_size,
			&r_addr,
			&r_size
		);

		SOCKADDR_IN address = { 0 };
		int addressSize = sizeof(address);

		int nRet = getpeername(m_ClientSocket, (struct sockaddr*)&address, &addressSize);

		if (nRet)
		{
			std::cerr << "Connection::GetIP : getpeername Failed\n";

			return false;
		}

		out_ = address.sin_addr.S_un.S_addr;

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

	std::atomic<bool> m_IsConnected;
	unsigned short m_ClientIndex;

	std::mutex m_mutex;

	char mAcceptBuf[64];

	stOverlappedEx m_RecvOverlapped;
	char mRecvBuf[MAX_SOCKBUF];

	RingBuffer m_SendBuffer;
	char m_SendingBuffer[MAX_SOCKBUF];
	stOverlappedEx m_SendOverlapped;
};