#pragma once

// 다중서버에 대한 세션을 MSSQL에서 Redis로 넘기자.
// 오브젝트풀, 커넥션풀을 최대한 활용

/*
* 유저가 할 수 있는 동작
* 1. 로비
* 로그인
* 회원가입
* 비밀번호 수정
* 캐릭터 목록 가져오기
* 캐릭터 정보 가져오기
* 캐릭터 선택하기 (가져오는 동작과 접속하는 동작은 분리한다.)
* 
* 2. 게임씬
* 스킬시전 -> 스킬코드 + 피격한 몬스터 목록(can be null)
* 아이템 줍기 -> 줍기 시도한 아이템의 맵상 오브젝트 인덱스
* 아이템 구매 -> 거래한 상인 + 구매할 아이템 코드
* 아이템 버리기 -> 버리려는 아이템의 인벤토리인덱스
* 
* 서버가 처리해야하는 동작
* 로그인 -> DB검색 -> true : Redis에 업로드 -> UserManager 반영 -> 결과 전송
*				   -> false : 결과 전송
* 회원가입 -> DB반영 -> 결과 전송
* 
* 비밀번호수정 -> DB반영 -> 결과 전송
* 
* 캐릭터 목록 가져오기 -> DB확인(Redis에 없다.) -> 결과 전송 (이 정보는 클라이언트가 가지고 있을 것.)
* 
* 캐릭터 정보 가져오기 -> DB확인(Redis에 아직 없을 수 있다.) -> 결과 전송 
* 
* 캐릭터 선택하기 -> Redis 확인 (클라이언트에서는 정보가져오기를 시도하고 선택하기를 고른다.) -> UserManager 반영
* 
* 스킬시전 -> User 정보에 맞는 데미지 계산 -> 몬스터 반영 -> Map에 있는 유저에게 전파
*													kill -> 아이템 드랍 -> Map의 유저에게 전파
* 
* 아이템 줍기 -> Map에서 해당 아이템 확인 및 제거 ->		해당 유저 : 결과 전송
*												  -> true : 다른 유저 : 해당 유저가 획득함 전파 (usercode, itemidx)
*												  -> false : -
* 
* 아이템 구매 -> DB확인 (가격정보는 아직 Redis에 없을 수 있다.) -> Redis에 분산락 걸고 데이터 변경 시도 -> 결과 전송 // 레디스는 트랜잭션 개념이 없어 락을 걸고 수정해야함.
* 
* 아이템 버리기 -> Redis에서 분산락 걸고 인벤토리를 수정한다 -> Map에 아이템 오브젝트를 생성한다. -> 모든 유저에게 전파한다.
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