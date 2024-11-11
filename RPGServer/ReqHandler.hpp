#pragma once
#include <string>
#include "Database.hpp"
#include <unordered_map>

/*
* Json -> param -> Operation
* 
* �� �������� �����ϴ� �� hashmap�� �̿��� �����Ѵ�. 
* 
* !! RESULTCODE::SUCCESS_WITH_ALREADY_RESPONSE !! -> �׳� �Լ� ������ �ϰ� ó���ϴ°ɷ� �ٲܱ�?
* ��� ������ ������ ������ �߰��� �����ؾ��ϱ� ������ �Լ� ���ο��� ����.
* ��ó���� �ʿ� ����.
* 
* 
* Todo. DropItem -> � �������̾������� ���� ������ �ʿ��ϴ�..
*/

const float DISTANCE_LIMIT_GET_OBJECT = 5.0f;

enum class RESULTCODE
{
	SUCCESS,
	SUCCESS_WITH_ALREADY_RESPONSE,	// �Լ� ���ο��� �޽����� ���±� ������ ���� ó���� �ʿ� ����.
	WRONG_PARAM,					// ��û��ȣ�� ���� �ʴ� �Ķ����
	SYSTEM_FAIL,					// �ý����� ����
	SIGNIN_FAIL,					// ID�� PW�� ���� ����
	SIGNIN_ALREADY_HAVE_SESSION,	// �̹� �α��ε� ����
	SIGNUP_FAIL,					// ID��Ģ�̳� PW��Ģ�� ���� ����
	SIGNUP_ALREADY_IN_USE,			// �̹� ������� ID
	WRONG_ORDER,					// ��û ������ ���� ���� (�����α����� ���� �ʾҴµ� ĳ�����ڵ带 ��û��)
	MODIFIED_MAPCODE,				// �ش� ��û�� ���� ���� ���ڵ�� ���� ������ ������ �ִ� �ش� ������ ���ڵ尡 ������.
	REQ_FAIL,						// ���� ���ǿ� ���� �ʾ� ������ ������ (�̹� ����� ������ ��..)
	UNDEFINED						// �˼����� ����
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

		// �Լ������͸� ���� ���
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

		// ���� Ʋ��
		if (nRet == CLIENT_NOT_CERTIFIED)
		{
			return RESULTCODE::SIGNIN_FAIL;
		}

		// �̹� �α����� ����
		if (nRet == ALREADY_HAVE_SESSION)
		{
			//��� ����
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
			return RESULTCODE::SYSTEM_FAIL;

		default:
			std::cerr << "ReqHandler::HandleSignUp : Undefined ReturnCode\n";
			return RESULTCODE::UNDEFINED;
		}
	}

	RESULTCODE HandleModifyPW(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		// Database �� �Լ� ���� ���� ��

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

		// ������ ������ ����

		return RESULTCODE::SUCCESS_WITH_ALREADY_RESPONSE;
	}

	RESULTCODE HandleGetCharInfo(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		GetCharInfoParameter stParam;
		m_JsonMaker.ToGetCharInfoParameter(param_, stParam);
		
		std::string strCharInfo = m_DB.GetCharInfoJsonStr(stParam.charcode);

		// ������ ������ ����

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

	// Ŭ���̾�Ʈ���� �ڱ� �������� ������ ������ �Ǵ��ص� �ȴ�.
	// �����Ǵ��� ���� �ȵ�� �ð������� �̿��� ������ �̿��� �ùķ��̼��� �ص� �ɵ�..
	// Ȥ�� �ش� ������ ����Ǵ� ���� �� �̵��� �Ͼ�� �����?
	// ���ڵ嵵 Ȯ���Ѵ�.
	RESULTCODE HandlePerformSkill(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		PerformSkillParameter stParam;
		m_JsonMaker.ToPerformSkillParameter(param_, stParam);

		// --ĳ���Ͱ� ��ų�� ����ߴٴ� ����� ����
		
		// ������ ���� ����
		if (stParam.monsterbitmask == 0)
		{
			return RESULTCODE::SUCCESS;
		}

		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		// --������ ��ų������ ������ ���� ������ ������� ������ ���


		// �ش� ��û�� ó���ϱ� ���� ���� �̵��Ǿ� �ݷ�.
		if (pUser->GetMapCode() != stParam.mapcode)
		{
			return RESULTCODE::MODIFIED_MAPCODE;
		}

		RPG::Map* pMap = m_MapManager.GetMap(pUser->GetMapCode());

		// --Ÿ���� ���� ������ �°� �ʿ� ���� (�ʿ��� ���� ������ó���� ���ó��, ��� ���� ����)

		return RESULTCODE::SUCCESS;
	}

	RESULTCODE HandleGetObject(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		GetObjectParameter stParam;
		m_JsonMaker.ToGetObjectParameter(param_, stParam);

		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		if (pUser->GetMapCode() != stParam.mapcode)
		{
			return RESULTCODE::MODIFIED_MAPCODE;
		}

		RPG::Map* pMap = m_MapManager.GetMap(pUser->GetMapCode());
		ItemObject* itemobj = pMap->PopObject(stParam.objectidx);

		// �̹� �����Ͽ��ų� ��ȿ�Ⱓ�� ���� ����� ������
		if (itemobj == nullptr)
		{
			// --���� ����

			return RESULTCODE::REQ_FAIL;
		}

		Vector2 position = itemobj->GetPosition();

		if (position.SquaredDistance(pUser->GetPosition()) <= DISTANCE_LIMIT_GET_OBJECT)
		{
			// --���� ����

			return RESULTCODE::REQ_FAIL;
		}

		// --���� ����

		// ��ü ��ȯ
		m_MapManager.ReleaseItemObject(itemobj);

		return RESULTCODE::SUCCESS;
	}

	RESULTCODE HandleBuyItem(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{


		return RESULTCODE::SUCCESS;
	}

	RESULTCODE HandleDropItem(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		DropItemParameter stParam;
		m_JsonMaker.ToDropItemParameter(param_, stParam);

		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		if (stParam.mapcode != pUser->GetMapCode())
		{
			// --��������
			return RESULTCODE::MODIFIED_MAPCODE;
		}

		int nRet = m_DB.DropItem(pUser->GetCharCode(), stParam.slotidx, stParam.count);

		if (nRet == 0)
		{
			// --��������
			return RESULTCODE::REQ_FAIL;
		}

		RPG::Map* pMap = m_MapManager.GetMap(stParam.mapcode);

		Vector2 position = pUser->GetPosition();

		pMap->CreateObject(nRet, stParam.count, 0, position);

		// --���� ���� (������ ���������� �ʿ��� ����. �ش� �޽����� �ش������� �κ��丮�� �ݿ��� ����.)

		return RESULTCODE::SUCCESS;
	}

	MapManager m_MapManager;
	JsonMaker m_JsonMaker;
	UserManager m_UserManager;
	Database m_DB;

	typedef RESULTCODE(ReqHandler::* REQ_HANDLE_FUNC)(const int, const unsigned int, const std::string&);

	std::unordered_map<MessageType, REQ_HANDLE_FUNC> Actions;
};