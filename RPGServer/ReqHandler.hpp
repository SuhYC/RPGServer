#pragma once
#include <string>
#include "Database.hpp"
#include <unordered_map>

/*
* Json -> param -> Operation
* 
* �� �������� �����ϴ� �� hashmap�� �̿��� �����Ѵ�. 
* (����Ʈ���� �� �������� �ֱ� �ϴ�.. �ؽ��Լ� ������ ������ ���� �ʿ�)
* 
* 
*/
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

		auto PacketFunc = [this](const unsigned short index_, const std::string& str_, PacketData* const out_) -> bool {return MakePacket(index_, str_, out_); };

		m_MapManager.MakePacketFunc = PacketFunc;
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

		auto itr = Actions.find(msg.type);

		if (itr == Actions.end())
		{
			std::cerr << "ReqHandler::HandleReq : Not Defined ReqType.\n";
			return;
		}

		// �Լ������͸� ���� ���
		(this->*(itr->second))(userindex_, msg.msg);

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

	MapManager m_MapManager;

private:
	bool HandleSignIn(const int userindex_, std::string& param_)
	{
		SignInParameter stParam;
		if (!m_JsonMaker.ToSignInParameter(param_, stParam))
		{
			return false;
		}

		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		int nRet = m_DB.SignIn(stParam.id, stParam.pw, pUser->GetIP());

		// ���� Ʋ��
		if (nRet == CLIENT_NOT_CERTIFIED)
		{
			// ��� ����
			return false;
		}

		// �̹� �α����� ����
		if (nRet == ALREADY_HAVE_SESSION)
		{
			//��� ����
			return false;
		}

		pUser->SetUserCode(nRet);
		// ��� ����
		return true;
	}

	bool HandleSignUp(const int userindex_, std::string& param_)
	{
		SignUpParameter stParam;
		if (!m_JsonMaker.ToSignUpParameter(param_, stParam))
		{
			return false;
		}

		eReturnCode eRet = m_DB.SignUp(stParam.id, stParam.pw);

		switch (eRet)
		{
		case eReturnCode::SIGNUP_SUCCESS:
			// ��� ����
			return true;
		case eReturnCode::SIGNUP_ALREADY_IN_USE:
			// ��� ����
			return false;
		case eReturnCode::SIGNUP_FAIL:
			// ��� ����
			return false;
		default:
			std::cerr << "ReqHandler::HandleSignUp : Undefined ReturnCode\n";
			return false;
		}
	}

	bool HandleModifyPW(const int userindex_, std::string& param_)
	{
		// Database �� �Լ� ���� ���� ��

		return true;
	}

	bool HandleGetCharList(const int userindex_, std::string& param_)
	{
		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		int usercode = pUser->GetUserCode();

		GetCharListParameter stParam;
		if (m_JsonMaker.ToGetCharListParameter(param_, stParam))
		{
			return false;
		}

		if (usercode == CLIENT_NOT_CERTIFIED || usercode != stParam.usercode)
		{
			return false;
		}

		std::string strCharList = m_DB.GetCharList(usercode);

		// ��� ����

		return true;
	}

	bool HandleGetCharInfo(const int userindex_, std::string& param_)
	{
		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		GetCharInfoParameter stParam;
		m_JsonMaker.ToGetCharInfoParameter(param_, stParam);
		
		std::string strCharInfo = m_DB.GetCharInfoJsonStr(stParam.charcode);

		// ��� ����

		return true;
	}

	bool HandleSelectChar(const int userindex_, std::string& param_)
	{
		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		SelectCharParameter stParam;
		m_JsonMaker.ToSelectCharParameter(param_, stParam);
		
		CharInfo* pInfo = m_DB.GetCharInfo(stParam.charcode);

		pUser->SetCharInfo(pInfo);

		RPG::Map* pMap = m_MapManager.GetMap(pInfo->LastMapCode);

		pMap->UserEnter(userindex_, pUser);

		// �Ϸ� ����

		return true;
	}

	bool HandlePerformSkill(const int userindex_, std::string& param_)
	{


		return true;
	}

	bool HandleGetObject(const int userindex_, std::string& param_)
	{


		return true;
	}

	bool HandleBuyItem(const int userindex_, std::string& param_)
	{


		return true;
	}

	bool HandleDropItem(const int userindex_, std::string& param_)
	{


		return true;
	}


	bool MakePacket(const unsigned short index_, const std::string& str_, PacketData* const out_)
	{
		if (out_ == nullptr)
		{
			return false;
		}

		char* msg = nullptr;

		try
		{
			msg = new char[str_.size() + 1];
		}
		catch (const std::bad_alloc& e)
		{
			std::cerr << "ReqHandler::MakePacket : �޸� ����\n";
			return false;
		}

		CopyMemory(msg, str_.c_str(), str_.size());
		msg[str_.size()] = NULL;

		out_->Init(index_, msg, str_.size());

		return true;
	}

	JsonMaker m_JsonMaker;
	UserManager m_UserManager;
	Database m_DB;

	typedef bool(ReqHandler::* REQ_HANDLE_FUNC)(const int, std::string&);

	std::unordered_map<MessageType, REQ_HANDLE_FUNC> Actions;
};