#pragma once
#include <string>
#include "Database.hpp"
#include <unordered_map>
#include "MapManager.hpp"
#include "UserManager.hpp"
#include "ChanceEvaluator.hpp"

/*
* Json -> param -> Operation
* 
* �� �������� �����ϴ� �� hashmap�� �̿��� �����Ѵ�. 
* 
* �� ������ �ڵ��ϴ� �Լ����� ��� �۽ű��� �Բ��ϱ�� ����.
* 
* BuyItem�� �ϴ� �ʸ��� NPC�� ��ġ�ϰ�
* 1. �÷��̾ ��ġ�� �ʿ� NPC�� �ִ��� Ȯ���ϰ� -> �ش� ���� DB�� �߰� �ʿ� [mapcode] [npccode]
* 2. NPC�� �Ĵ� ��ǰ�� �ش� �������� �ִ��� Ȯ���ϰ� -> �ش� ���� DB�� �߰� �ʿ� [npccode] [itemcode]
* 3. �ش� �������� ���ݰ� ���Ͽ� ������ �� �ִ��� (�ϴ� DB�� ���� ������ Ȯ���ϰ� Redis�� ������û�ϴ� �κ��� �����ξ���.)
* Ȯ���ϴ� ������� �ϸ� ���� �� ����.
* ���� �� ����غ��� ����.
* 
* MoveMap�� DB�� �ʰ� ���������� ����ϰ� -> DB�� �߰� �ʿ� [mapcode] [mapcode]
* �̸� ������ �����Ҷ� �޾ƿ� DB�� ������ �ִ� ������ �Ѵ�. (MapEdge)
* �̵����ɿ��θ� DB�κ��� Ȯ�ιް� �����Ѵ�.
* 
* todo. DB�� npc-item ������ -> npc�� �ش� �������� �Ǹ��Ѵٴ� ����
* map-npc ������ -> �ش� �ʿ� npc�� �����Ѵٴ� ����
* item-price������ -> �ش� �������� �⺻ ���� ���Ű� ����
* map-map������ ����Ͽ�����. -> �ش� �ʿ��� ���� ������ �̵��� �� �ִٴ� ����
*/


class ReqHandler
{
public:
	ReqHandler()
	{
		Actions = std::unordered_map<MessageType, REQ_HANDLE_FUNC>();

		// ----- �ƿ����� -----
		Actions[MessageType::SIGNIN] = &ReqHandler::HandleSignIn;
		Actions[MessageType::SIGNUP] = &ReqHandler::HandleSignUp;
		Actions[MessageType::MODIFY_PW] = &ReqHandler::HandleModifyPW;
		Actions[MessageType::GET_CHAR_LIST] = &ReqHandler::HandleGetCharList;
		Actions[MessageType::GET_CHAR_INFO] = &ReqHandler::HandleGetCharInfo; // ���� �̰� �ΰ��ӿ��� Ÿ���� ������ �������µ��� ����Ѵ�.
		Actions[MessageType::RESERVE_CHAR_NAME] = &ReqHandler::HandleReserveCharName;
		Actions[MessageType::CANCEL_CHAR_NAME_RESERVE] = &ReqHandler::HandleCancelCharNameReserve;
		Actions[MessageType::CREATE_CHAR] = &ReqHandler::HandleCreateChar;
		Actions[MessageType::SELECT_CHAR] = &ReqHandler::HandleSelectChar;

		// ----- �ΰ��� -----
		Actions[MessageType::PERFORM_SKILL] = &ReqHandler::HandlePerformSkill;
		Actions[MessageType::GET_OBJECT] = &ReqHandler::HandleGetObject;
		Actions[MessageType::BUY_ITEM] = &ReqHandler::HandleBuyItem;
		Actions[MessageType::DROP_ITEM] = &ReqHandler::HandleDropItem;
		Actions[MessageType::USE_ITEM] = &ReqHandler::HandleUseItem;
		Actions[MessageType::MOVE_MAP] = &ReqHandler::HandleMoveMap;
		Actions[MessageType::CHAT_EVERYONE] = &ReqHandler::HandleChatEveryone;
		Actions[MessageType::POS_INFO] = &ReqHandler::HandlePosInfo;
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

	// ���� ����� �ǽɽ����� ������ ����ȴٸ� ���� ����ϴ°͵� ���ڴ�.
	void HandleReq(const int userindex_, std::string& Req_)
	{
		ReqMessage msg;
		if (!m_JsonMaker.ToReqMessage(Req_, msg))
		{
			// ReqMessage�������� �ȸ���ٰ�..?
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

		CharInfo* pInfo = m_UserManager.ReleaseUser(connidx_);

		// ���������� ����ϰ� ��ü�� ��ȯ�Ѵ�.
		if (pInfo != nullptr)
		{
			m_DB.UpdateCharInfo(pInfo);
			m_DB.ReleaseObject(pInfo);
		}

		return;
	}

	void SetIP(const int connidx_, const uint32_t ip_)
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
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM);
		}

		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		int nRet = m_DB.SignIn(stParam.id, stParam.pw, pUser->GetIP());

		// ���� Ʋ��
		if (nRet == CLIENT_NOT_CERTIFIED)
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::SIGNIN_FAIL);
		}

		// �̹� �α����� ����
		if (nRet == ALREADY_HAVE_SESSION)
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::SIGNIN_ALREADY_HAVE_SESSION);
		}

		pUser->SetUserCode(nRet);

		return SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS);
	}

	RESULTCODE HandleSignUp(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		SignUpParameter stParam;
		if (!m_JsonMaker.ToSignUpParameter(param_, stParam))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM);
		}

		eReturnCode eRet = m_DB.SignUp(stParam.id, stParam.pw, stParam.questno, stParam.ans, stParam.hint);

		switch (eRet)
		{
		case eReturnCode::SIGNUP_SUCCESS:
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS);

		case eReturnCode::SIGNUP_ALREADY_IN_USE:
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::SIGNUP_ALREADY_IN_USE);

		case eReturnCode::SIGNUP_FAIL:
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::SIGNUP_FAIL);

		case eReturnCode::SYSTEM_ERROR:
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::SYSTEM_FAIL);

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
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM);
		}

		if (usercode == CLIENT_NOT_CERTIFIED || usercode != stParam.usercode)
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_ORDER);
		}

		std::string strCharList = m_DB.GetCharList(usercode);

		return SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS, strCharList);
	}

	RESULTCODE HandleGetCharInfo(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		GetCharInfoParameter stParam;
		if (!m_JsonMaker.ToGetCharInfoParameter(param_, stParam))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM);
		}
		
		std::string strCharInfo = m_DB.GetCharInfoJsonStr(stParam.charcode);

		return SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS, strCharInfo);
	}

	RESULTCODE HandleReserveCharName(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		ReserveCharNameParameter stParam;
		if (!m_JsonMaker.ToReserveCharNameParameter(param_, stParam))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM);
		}

		if (!m_DB.ReserveNickname(stParam.CharName))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::REQ_FAIL);
		}

		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		pUser->SetReserveNickname(stParam.CharName);

		return SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS);
	}

	RESULTCODE HandleCancelCharNameReserve(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		// ���� �Ķ���͸� ����Ѵ�.
		ReserveCharNameParameter stParam;
		if (!m_JsonMaker.ToReserveCharNameParameter(param_, stParam))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM);
		}

		if (stParam.CharName.empty())
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM);
		}

		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		if (pUser->GetReserveNickname() != stParam.CharName)
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::REQ_FAIL);
		}

		if (!m_DB.CancelReserveNickname(stParam.CharName))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::REQ_FAIL);
		}

		return SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS);
	}

	RESULTCODE HandleCreateChar(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		CreateCharParameter stParam;
		if (!m_JsonMaker.ToCreateCharParameter(param_, stParam))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM);
		}

		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		// ������ ���� Ȥ�� �߸��� ��û
		// Ȥ�� ���� �б⿡ �ɸ����� �𸣴� �������� �Ǵ��ϴ� �ð����� Ŭ���̾�Ʈ���� �Ǵ��ϴ� ��ȿ�Ⱓ�� ª�� �� ��.
		if (pUser->GetReserveNickname() != stParam.CharName)
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::REQ_FAIL);
		}

		// ���۸� �ڵ带 ���� �����ߴ�. �����ʿ��� �����ϰ� �ұ� �ƴϸ� �� �ʿ����� �����ұ�
		if (!m_DB.CreateCharactor(pUser->GetUserCode(), stParam.CharName, 0))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::SYSTEM_FAIL);
		}

		return SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS);
	}

	RESULTCODE HandleSelectChar(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		SelectCharParameter stParam;
		if (!m_JsonMaker.ToSelectCharParameter(param_, stParam))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM);
		}

		std::string strList = m_DB.GetCharList(pUser->GetUserCode());
		CharList stCharList;
		if (!m_JsonMaker.ToCharList(strList, stCharList))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::SYSTEM_FAIL);
		}

		// ���� ���� ����
		if (!stCharList.find(stParam.charcode))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_ORDER);
		}

		CharInfo* pInfo = m_DB.GetCharInfo(stParam.charcode);

		pUser->SetCharInfo(pInfo);

		RPG::Map* pMap = m_MapManager.GetMap(pInfo->LastMapCode);

		// �� ���� ���� �ʱ�ȭ �ʿ�
		if (pMap->IsNew())
		{
			std::vector<MonsterSpawnInfo> spawnInfo = m_DB.GetMonsterSpawnInfo(pInfo->LastMapCode);

			for (size_t i = 0; i < spawnInfo.size(); i++)
			{
				pMap->InitSpawnInfo(i, spawnInfo[i]);
			}
		}

		pMap->UserEnter(userindex_, pUser);

		return SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS);
	}

	// Ŭ���̾�Ʈ���� �ڱ� �������� �ð������� �̿��� ������ �̿��� �ùķ��̼��� �ص� �ɵ�..
	RESULTCODE HandlePerformSkill(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		PerformSkillParameter stParam;
		if (!m_JsonMaker.ToPerformSkillParameter(param_, stParam))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM);
		}

		// --ĳ���Ͱ� ��ų�� ����ߴٴ� ����� ���� (GameMap�� SendToAll�� ����� �������.)

		// ������ ���� ����
		if (stParam.monsterbitmask == 0)
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS);
		}

		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		// �ϴ� ���ذ��� ����ϰ�, ���Ϳ� ������ �� ������ ������.
		long long damage = pUser->calStatDamage(); 

		// �ش� ��û�� ó���ϱ� ���� ���� �̵��Ǿ� �ݷ�.
		if (pUser->GetMapCode() != stParam.mapcode)
		{
			return RESULTCODE::MODIFIED_MAPCODE;
		}

		RPG::Map* pMap = m_MapManager.GetMap(pUser->GetMapCode());

		stParam.monsterbitmask;

		for (int bit = 1, cnt = 0; cnt < RPG::MAX_MONSTER_COUNT_ON_MAP; cnt++, bit *= 2)
		{
			// �ش� ��Ʈ�� ���ʹ� Ÿ�ݴ���� �ƴ�.
			if ((stParam.monsterbitmask & bit) == 0)
			{
				continue;
			}

			// �ϴ� 0.75 ~ 1.25�� ���� ���ؼ� ���������� �������� ���Ѵ�.
			long long finalDamage = ChanceEvaluator::GetInstance().CalDamage(damage, 0.25);

			if (finalDamage == 0)
			{
				continue;
			}

			auto pair = pMap->AttackMonster(userindex_, cnt, finalDamage);

			if (pair.first != 0)
			{
				// --�ش� ���Ϳ� �������� �־����� ����
			}

			if (pair.second != 0)
			{
				// --�ش� �ڵ忡 �´� ������ ����
				// --�ش� �ʿ� ���ó�� ����
			}
		}

		return RESULTCODE::SUCCESS;
	}

	RESULTCODE HandleGetObject(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		GetObjectParameter stParam;
		m_JsonMaker.ToGetObjectParameter(param_, stParam);

		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		if (pUser->GetMapCode() != stParam.mapcode)
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::MODIFIED_MAPCODE);
		}

		RPG::Map* pMap = m_MapManager.GetMap(pUser->GetMapCode());
		ItemObject* itemobj = pMap->PopObject(stParam.objectidx, pUser->GetCharCode());

		// �̹� �����Ͽ��ų� ������ų� �Ÿ��� �ְų� ��������� ���� ������
		if (itemobj == nullptr)
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::REQ_FAIL);
		}

		// ��ü ��ȯ
		m_MapManager.ReleaseItemObject(itemobj);

		return SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS);
	}

	RESULTCODE HandleBuyItem(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		BuyItemParameter stParam;
		if (!m_JsonMaker.ToBuyItemParameter(param_, stParam))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM);
		}

		// 1. �������� Ȯ��
		
		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		// 2. ĳ���� ��ġ Ȯ��
		
		RPG::Map* pMap = m_MapManager.GetMap(pUser->GetMapCode());
		if (pMap == nullptr)
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_ORDER);
		}

		// 3. �ش� ��ġ�� npc Ȯ��
		
		if (!m_DB.FindNPCInfo(pUser->GetMapCode(), stParam.npccode))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_ORDER);
		}

		// 4. npc�� �ǸŸ�� Ȯ�� : ��ȿ�Ⱓ�� Ȯ���Ұ�.
	
		// �ǸŸ�Ͽ� ����
		if (!m_DB.FindSalesList(stParam.npccode, stParam.itemcode))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_ORDER);
		}

		// 5. ������ ���� �õ� (DB -> REDIS)

		// ������ ���� Ȥ�� �κ��丮 ���� ������ ����
		if (!m_DB.BuyItem(pUser->GetCharCode(), stParam.itemcode, 0, stParam.count))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::REQ_FAIL);
		}

		return SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS);
	}

	RESULTCODE HandleDropItem(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		DropItemParameter stParam;
		m_JsonMaker.ToDropItemParameter(param_, stParam);

		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		if (stParam.mapcode != pUser->GetMapCode())
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::MODIFIED_MAPCODE);
		}

		int nRet = m_DB.DropItem(pUser->GetCharCode(), stParam.slotidx, stParam.count);

		if (nRet == 0)
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::REQ_FAIL);
		}

		RPG::Map* pMap = m_MapManager.GetMap(stParam.mapcode);

		Vector2 position = pUser->GetPosition();

		// ������Ʈ�� ������ ���� �����ϴ°� Map�̴�.
		pMap->CreateObject(nRet, stParam.count, 0, position);

		return SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS);
	}

	RESULTCODE HandleUseItem(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		// DB�� ���ȿ������ ���̺� �����߰ڴ�.
		// HPȸ�� MPȸ�� STR���� DEX���� INT���� MEN���� PHYSICATT���� MAGICATT���� ...

		UseItemParameter stParam;

		if (!m_JsonMaker.ToUseItemParameter(param_, stParam))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM);
		}




		return RESULTCODE::SUCCESS;
	}

	RESULTCODE HandleMoveMap(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		MoveMapParameter stParam;
		if (!m_JsonMaker.ToMoveMapParameter(param_, stParam))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM);
		}

		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);
		
		// �ش� �ʿ��� ��û�� ������ �� �� ����
		if (!m_DB.FindMapEdge(pUser->GetMapCode(), stParam.tomapcode))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_ORDER);
		}

		// �� �̵�
		RPG::Map* pMap = m_MapManager.GetMap(pUser->GetMapCode());
		pMap->UserExit(userindex_);
		pMap = m_MapManager.GetMap(stParam.tomapcode);

		// �ش� �ʿ� ó�� ������ ��� �ʱ�ȭ�� ���־���Ѵ�.
		if (pMap->IsNew())
		{
			// --�ʱ�ȭ
		}

		pMap->UserEnter(userindex_, pUser);

		return SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS);
	}

	RESULTCODE HandleChatEveryone(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		ChatEveryoneParameter stParam;

		if (!m_JsonMaker.ToChatEveryoneParameter(param_, stParam))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM);
		}

		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		RPG::Map* pMap = m_MapManager.GetMap(pUser->GetMapCode());

		if (pMap == nullptr)
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_ORDER);
		}

		// --�ش� �ʿ� ����

		return RESULTCODE::SUCCESS;
	}

	// �ش� ó���� Ŭ���̾�Ʈ���� �����ȯ�� ���� �ʴ´�.
	RESULTCODE HandlePosInfo(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		PosInfoParameter stParam;
		if (!m_JsonMaker.ToPosInfoParameter(param_, stParam))
		{

			return RESULTCODE::WRONG_PARAM;
		}

		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		pUser->UpdatePosition(stParam.pos, stParam.vel);

		return RESULTCODE::SUCCESS;
	}

	RESULTCODE SendResultMsg(const int userIndex_, const unsigned int ReqNo_,
		RESULTCODE resCode_, std::string& optionalMsg_)
	{
		ResMessage stResultMsg{ ReqNo_, resCode_, optionalMsg_ };
		std::string jsonmsg;
		if (!m_JsonMaker.ToJsonString(stResultMsg, jsonmsg))
		{
			std::cerr << "ReqHandler::SendResultMsg : Failed to Make Jsonstr\n";
			return RESULTCODE::UNDEFINED;
		}

		PacketData* packet = AllocatePacket();

		packet->Init(userIndex_, jsonmsg);
		
		if (!SendMsgFunc(packet))
		{
			// �۽� ����
			return RESULTCODE::UNDEFINED;
		}

		// �۽� ����
		return resCode_;
	}
	
	RESULTCODE SendResultMsg(const int userIndex_, const unsigned int ReqNo_,
		RESULTCODE resCode_, std::string&& optionalMsg_ = std::string())
	{
		ResMessage stResultMsg{ ReqNo_, resCode_, optionalMsg_ };
		std::string jsonmsg;
		if (!m_JsonMaker.ToJsonString(stResultMsg, jsonmsg))
		{
			std::cerr << "ReqHandler::SendResultMsg : Failed to Make Jsonstr\n";
			return RESULTCODE::UNDEFINED;
		}

		PacketData* packet = AllocatePacket();

		packet->Init(userIndex_, jsonmsg);

		if (!SendMsgFunc(packet))
		{
			// �۽� ����
			return RESULTCODE::UNDEFINED;
		}

		// �۽� ����
		return resCode_;
	}

	MapManager m_MapManager;
	JsonMaker m_JsonMaker;
	UserManager m_UserManager;
	Database m_DB;

	typedef RESULTCODE(ReqHandler::* REQ_HANDLE_FUNC)(const int, const unsigned int, const std::string&);

	std::unordered_map<MessageType, REQ_HANDLE_FUNC> Actions;
};