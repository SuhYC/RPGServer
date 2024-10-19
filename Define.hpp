#pragma once

#include <WinSock2.h>
#include <MSWSock.h> // AcceptEx()
#include <WS2tcpip.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib") // acceptEx()

const unsigned short MAX_SOCKBUF = 100;
const unsigned short MAX_WORKTHREAD = 4;
const unsigned long LIFE_OF_TOKEN_SEC = 60;

enum class eIOOperation
{
	RECV,
	SEND,
	ACCEPT
};

struct stOverlappedEx
{
	WSAOVERLAPPED m_overlapped;
	unsigned short m_userIndex;
	WSABUF m_wsaBuf;
	eIOOperation m_eOperation;
};

