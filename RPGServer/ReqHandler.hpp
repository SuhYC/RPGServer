#pragma once
#include <string>
#include "Database.hpp"
#include <unordered_map>

/*
* Json -> param -> Operation
* 
* 각 동작으로 연결하는 건 hashmap을 이용해 연결한다. 
* 
* !! RESULTCODE::SUCCESS_WITH_ALREADY_RESPONSE !! -> 그냥 함수 내에서 일괄 처리하는걸로 바꿀까?
* 결과 통지에 별도의 정보를 추가로 전달해야하기 때문에 함수 내부에서 전달.
* 후처리가 필요 없음.
* 
* 
*/

enum class RESULTCODE
{
	SUCCESS,
	SUCCESS_WITH_ALREADY_RESPONSE, // 함수 내부에서 메시지를 보냈기 때문에 따로 처리할 필요 없음.
	WRONG_PARAM, // 요청번호와 맞지 않는 파라미터
	FAIL, // 시스템의 문제
	SIGNIN_FAIL, // ID와 PW가 맞지 않음
	SIGNIN_ALREADY_HAVE_SESSION, // 이미 로그인된 계정
	SIGNUP_FAIL, // ID규칙이나 PW규칙이 맞지 않음
	SIGNUP_ALREADY_IN_USE, // 이미 사용중인 ID
	WRONG_ORDER, // 요청 서순이 맞지 않음 (유저로그인이 되지 않았는데 캐릭터코드를 요청함)
	UNDEFINED // 알수없는 오류
};

class ReqHandler
{
public:
	ReqHandler()
	{
		Actions = std::unordered_map<MessageType, REQ_HANDLE_FUNC>();
		Actions[MessageType::SIGNIN] = &ReqHandler::HandleSignIn;
		Actions[MessageType::SIGNUP] = &ReqHandler::HandleSignUp;
		Actions[MessageType::MODIFY_PW] = &ReqHandler::HandleModifyPW;
		Actions[MessageType::GET_CHAR_LIST] = &ReqHandler::HandleGetCharList;
		Actions[MessageType::GET_CHAR_INFO] = &ReqHandler::HandleGetCharInfo;
		Actions[MessageType::SELECT_CHAR] = &ReqHandler::HandleSelectChar;

		Actions[MessageType::PERFORM_SKILL] = &ReqHandler::HandlePerformSkill;
		Actions[MessageType::GET_OBJECT] = &ReqHandler::HandleGetObject;
		Actions[MessageType::BUY_ITEM] = &ReqHandler::HandleBuyItem;
		Actions[MessageType::DROP_ITEM] = &ReqHandler::HandleDropItem;
	}

	void Init(const unsigned short MaxClient_)
	{
		auto releaseInfoFunc = [this](CharInfo* pInfo_) {m_DB.ReleaseObject(pInfo_); };
		m_UserManager.ReleaseInfo = releaseInfoFunc;
		m_UserManager.Init(MaxClient_);

		m_MapManager.AllocatePacket = this->AllocatePacket;
		m_MapManager.DeallocatePacket = this->DeallocatePacket;
		m_MapManager.SendMsgFunc = this->SendMsgFunc;
	}

	~ReqHandler()
	{
		m_UserManager.Clear();
	}

	void HandleReq(const int userindex_, std::string& Req_)
	{
		ReqMessage msg;
		if (!m_JsonMaker.ToReqMessage(Req_, msg))
		{

			return;
		}

		auto itr = Actions.find(msg.type);

		if (itr == Actions.end())
		{
			std::cerr << "ReqHandler::HandleReq : Not Defined ReqType.\n";
			return;
		}

		// 함수포인터를 통한 사용
		RESULTCODE eRet = (this->*(itr->second))(userindex_, msg.reqNo, msg.msg);

		return;
	}

	void HandleLogOut(const int connidx_)
	{
		User* pUser = m_UserManager.GetUserByConnIndex(connidx_);
		int usercode = pUser->GetUserCode();

		if (usercode != CLIENT_NOT_CERTIFIED)
		{
			m_DB.LogOut(usercode);
		}

		m_UserManager.ReleaseUser(connidx_);

		return;
	}

	void SetIP(const int connidx_, std::string& ip_)
	{
		User* pUser = m_UserManager.GetUserByConnIndex(connidx_);
		pUser->SetIP(ip_);

		return;
	}

	std::function<PacketData* ()> AllocatePacket;
	std::function<void(PacketData*)> DeallocatePacket;
	std::function<bool(PacketData*)> SendMsgFunc;

private:
	RESULTCODE HandleSignIn(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		SignInParameter stParam;
		if (!m_JsonMaker.ToSignInParameter(param_, stParam))
		{
			return RESULTCODE::WRONG_PARAM;
		}

		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		int nRet = m_DB.SignIn(stParam.id, stParam.pw, pUser->GetIP());

		// 정보 틀림
		if (nRet == CLIENT_NOT_CERTIFIED)
		{
			return RESULTCODE::SIGNIN_FAIL;
		}

		// 이미 로그인한 계정
		if (nRet == ALREADY_HAVE_SESSION)
		{
			//결과 전송
			return RESULTCODE::SIGNIN_ALREADY_HAVE_SESSION;
		}

		pUser->SetUserCode(nRet);
		return RESULTCODE::SUCCESS;
	}

	RESULTCODE HandleSignUp(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		SignUpParameter stParam;
		if (!m_JsonMaker.ToSignUpParameter(param_, stParam))
		{
			return RESULTCODE::WRONG_PARAM;
		}

		eReturnCode eRet = m_DB.SignUp(stParam.id, stParam.pw);

		switch (eRet)
		{
		case eReturnCode::SIGNUP_SUCCESS:
			return RESULTCODE::SUCCESS;

		case eReturnCode::SIGNUP_ALREADY_IN_USE:
			return RESULTCODE::SIGNUP_ALREADY_IN_USE;

		case eReturnCode::SIGNUP_FAIL:
			return RESULTCODE::SIGNUP_FAIL;

		case eReturnCode::SYSTEM_ERROR:
			return RESULTCODE::FAIL;

		default:
			std::cerr << "ReqHandler::HandleSignUp : Undefined ReturnCode\n";
			return RESULTCODE::UNDEFINED;
		}
	}

	RESULTCODE HandleModifyPW(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		// Database 쪽 함수 먼저 만들 것

		return RESULTCODE::SUCCESS;
	}

	RESULTCODE HandleGetCharList(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		int usercode = pUser->GetUserCode();

		GetCharListParameter stParam;
		if (!m_JsonMaker.ToGetCharListParameter(param_, stParam))
		{
			return RESULTCODE::WRONG_PARAM;
		}

		if (usercode == CLIENT_NOT_CERTIFIED || usercode != stParam.usercode)
		{
			return RESULTCODE::WRONG_ORDER;
		}

		std::string strCharList = m_DB.GetCharList(usercode);

		// 별도의 데이터 전송

		return RESULTCODE::SUCCESS_WITH_ALREADY_RESPONSE;
	}

	RESULTCODE HandleGetCharInfo(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		GetCharInfoParameter stParam;
		m_JsonMaker.ToGetCharInfoParameter(param_, stParam);
		
		std::string strCharInfo = m_DB.GetCharInfoJsonStr(stParam.charcode);

		// 별도의 데이터 전송

		return RESULTCODE::SUCCESS_WITH_ALREADY_RESPONSE;
	}

	RESULTCODE HandleSelectChar(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		SelectCharParameter stParam;
		m_JsonMaker.ToSelectCharParameter(param_, stParam);
		
		CharInfo* pInfo = m_DB.GetCharInfo(stParam.charcode);

		pUser->SetCharInfo(pInfo);

		RPG::Map* pMap = m_MapManager.GetMap(pInfo->LastMapCode);

		pMap->UserEnter(userindex_, pUser);

		return RESULTCODE::SUCCESS;
	}

	RESULTCODE HandlePerformSkill(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{


		return RESULTCODE::SUCCESS;
	}

	RESULTCODE HandleGetObject(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{


		return RESULTCODE::SUCCESS;
	}

	RESULTCODE HandleBuyItem(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{


		return RESULTCODE::SUCCESS;
	}

	RESULTCODE HandleDropItem(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{


		return RESULTCODE::SUCCESS;
	}

	MapManager m_MapManager;
	JsonMaker m_JsonMaker;
	UserManager m_UserManager;
	Database m_DB;

	typedef RESULTCODE(ReqHandler::* REQ_HANDLE_FUNC)(const int, const unsigned int, const std::string&);

	std::unordered_map<MessageType, REQ_HANDLE_FUNC> Actions;
};