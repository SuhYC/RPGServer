#pragma once
#include <string>
#include "Database.hpp"
#include <unordered_map>

/*
* Json -> param -> Operation
* 
* 각 동작으로 연결하는 건 hashmap을 이용해 연결한다. 
* (이진트리가 더 빠를수도 있긴 하다.. 해시함수 성능이 얼마인지 조사 필요)
* 
* 
*/
class ReqHandler
{
public:
	ReqHandler()
	{
		Actions.emplace(MessageType::SIGNIN, HandleSignIn);
		Actions.emplace(MessageType::SIGNUP, HandleSignUp);
		Actions.emplace(MessageType::MODIFY_PW, HandleModifyPW);
		Actions.emplace(MessageType::GET_CHAR_LIST, HandleGetCharList);
		Actions.emplace(MessageType::GET_CHAR_INFO, HandleGetCharInfo);
		Actions.emplace(MessageType::SELECT_CHAR, HandleSelectChar);

		Actions.emplace(MessageType::PERFORM_SKILL, HandlePerformSkill);
		Actions.emplace(MessageType::GET_OBJECT, HandleGetObject);
		Actions.emplace(MessageType::BUY_ITEM, HandleBuyItem);
		Actions.emplace(MessageType::DROP_ITEM, HandleDropItem);
	}

	void Init(const unsigned short MaxClient_)
	{
		m_UserManager.Init(MaxClient_);
	}

	~ReqHandler()
	{
		m_UserManager.Clear();
	}

	void HandleReq(const int userindex_, std::string& Req_)
	{
		ReqMessage msg;
		m_JsonMaker.ToReqMessage(Req_, msg);

		Actions[msg.type](userindex_, msg.msg);

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

private:
	bool HandleSignIn(const int userindex_, std::string& param_)
	{
		SignInParameter stParam;
		m_JsonMaker.ToSignInParameter(param_, stParam);

		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		int nRet = m_DB.SignIn(stParam.id, stParam.pw, pUser->GetIP());

		// 정보 틀림
		if (nRet == CLIENT_NOT_CERTIFIED)
		{
			// 결과 전송
			return false;
		}

		// 이미 로그인한 계정
		if (nRet == ALREADY_HAVE_SESSION)
		{
			//결과 전송
			return false;
		}

		pUser->SetUserCode(nRet);
		// 결과 전송
		return true;
	}

	bool HandleSignUp(const int userindex_, std::string& param_)
	{
		SignUpParameter stParam;
		m_JsonMaker.ToSignUpParameter(param_, stParam);

		eReturnCode eRet = m_DB.SignUp(stParam.id, stParam.pw);

		switch (eRet)
		{
		case eReturnCode::SIGNUP_SUCCESS:
			// 결과 전송
			return true;
		case eReturnCode::SIGNUP_ALREADY_IN_USE:
			// 결과 전송
			return false;
		case eReturnCode::SIGNUP_FAIL:
			// 결과 전송
			return false;
		default:
			std::cerr << "ReqHandler::HandleSignUp : Undefined ReturnCode\n";
			return false;
		}
	}

	bool HandleModifyPW(const int userindex_, std::string& param_)
	{
		// Database 쪽 함수 먼저 만들 것
	}

	bool HandleGetCharList(const int userindex_, std::string& param_)
	{
		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		int usercode = pUser->GetUserCode();

		if (usercode == CLIENT_NOT_CERTIFIED)
		{
			return false;
		}

		CharList* pCharList = m_DB.GetCharList(usercode);
		// redis 확인, redis 등록 과정이 없다. 다시 확인하기
	}

	bool HandleGetCharInfo(const int userindex_, std::string& param_)
	{

	}

	bool HandleSelectChar(const int userindex_, std::string& param_)
	{

	}

	bool HandlePerformSkill(const int userindex_, std::string& param_)
	{

	}

	bool HandleGetObject(const int userindex_, std::string& param_)
	{

	}

	bool HandleBuyItem(const int userindex_, std::string& param_)
	{

	}

	bool HandleDropItem(const int userindex_, std::string& param_)
	{

	}

	JsonMaker m_JsonMaker;
	UserManager m_UserManager;
	Database m_DB;
	std::unordered_map<MessageType, std::function<bool(const int, std::string)>> Actions;
};