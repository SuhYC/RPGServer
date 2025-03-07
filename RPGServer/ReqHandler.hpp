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
		Actions[MessageType::GET_INVEN] = &ReqHandler::HandleGetInven;
		Actions[MessageType::GET_SALESLIST] = &ReqHandler::HandleGetSalesList;
		Actions[MessageType::SWAP_INVENTORY] = &ReqHandler::HandleSwapInven;

		// ----- �ΰ��� -----
		Actions[MessageType::PERFORM_SKILL] = &ReqHandler::HandlePerformSkill;
		Actions[MessageType::GET_OBJECT] = &ReqHandler::HandleGetObject;
		Actions[MessageType::BUY_ITEM] = &ReqHandler::HandleBuyItem;
		Actions[MessageType::DROP_ITEM] = &ReqHandler::HandleDropItem;
		Actions[MessageType::USE_ITEM] = &ReqHandler::HandleUseItem;
		Actions[MessageType::MOVE_MAP] = &ReqHandler::HandleMoveMap;
		Actions[MessageType::CHAT_EVERYONE] = &ReqHandler::HandleChatEveryone;
		Actions[MessageType::POS_INFO] = &ReqHandler::HandlePosInfo;

		// ----- ����� �׽�Ʈ�� (������ ���� �� ����) -----
		Actions[MessageType::DEBUG_SET_GOLD] = &ReqHandler::HandleSetGold;
	}

	void Init(const unsigned short MaxClient_)
	{
		auto releaseInfoFunc = [this](CharInfo* pInfo_) {m_DB.ReleaseObject(pInfo_); };
		m_UserManager.ReleaseInfo = releaseInfoFunc;
		m_UserManager.Init(MaxClient_);

		auto SendInfo = [this](const int userindex_, RESULTCODE rescode_, std::string& msg_) {SendInfoMsg(userindex_, rescode_, msg_); };
		auto SendInfoToUsers = [this](std::map<int, User*>& users, RESULTCODE rescode_, std::string& msg_, int exceptUsercode_ = 0) {SendInfoMsgToUsers(users, rescode_, msg_, exceptUsercode_); };
		m_MapManager.SendInfoFunc = SendInfo;
		m_MapManager.SendInfoToUsersFunc = SendInfoToUsers;
	}

	~ReqHandler()
	{
		m_UserManager.Clear();
	}

	// ���� ����� �ǽɽ����� ������ ����ȴٸ� ���� ����ϴ°͵� ���ڴ�.
	bool HandleReq(const int userindex_, std::string& Req_)
	{
		ReqMessage msg;
		if (!m_JsonMaker.ToReqMessage(Req_, msg))
		{
			std::cout << "ReqHandler::HandleReq : ToReqMsg ����\n";
			// ReqMessage�������� �ȸ���ٰ�..?
			return false;
		}

		auto itr = Actions.find(msg.type);

		if (itr == Actions.end())
		{
			std::cerr << "ReqHandler::HandleReq : Not Defined ReqType.\n";
			return false;
		}

		// �Լ������͸� ���� ���
		RESULTCODE eRet = (this->*(itr->second))(userindex_, msg.reqNo, msg.msg);

		return true;
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
			if (pInfo->CharNo != CLIENT_NOT_CERTIFIED)
			{
				m_DB.UpdateInventory(pInfo->CharNo);
			}

			RenewInfo(*pInfo);

			RPG::Map* pMap = m_MapManager.GetMap(pInfo->LastMapCode);

			pMap->UserExit(connidx_);

			m_DB.UpdateCharInfo(pInfo);
			m_DB.ReleaseObject(pInfo);
		}


		return;
	}

	void ClearSession()
	{
		for (int idx = 0; idx < m_MaxClient; idx++)
		{
			HandleLogOut(idx);
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

		if (pUser->GetUserCode() != CLIENT_NOT_CERTIFIED)
		{
			HandleLogOut(userindex_);
		}

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

		std::string strCharList;
		try
		{
			strCharList = m_DB.GetCharList(nRet);
		}
		catch (ToJsonException e)
		{
			std::cerr << "ReqHandler::HandleSignin : ToJsonException : " << e.what() << '\n';
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::UNDEFINED);
		}

		return SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS, strCharList);
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

	/// <summary>
	/// SignIn�̶� ������ ������ ������� �ߴµ�
	/// SignIn �Ŀ� ������ CharList�� ��û�Ͽ����Ѵ�.
	/// SignIn ���� ������ �����Ǿ� �ִ�.
	/// GetCharList�� ���������� ��������� ���� �𸣰���.
	/// </summary>
	/// <param name="userindex_"></param>
	/// <param name="ReqNo"></param>
	/// <param name="param_"></param>
	/// <returns></returns>
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

		std::string strCharList;
		try
		{
			strCharList = m_DB.GetCharList(usercode);
		}
		catch (ToJsonException e)
		{
			std::cerr << "ReqHandler::HandleGetCharList : ToJsonException : " << e.what() << '\n';
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::UNDEFINED);
		}

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

		if (stParam.CharName.empty())
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
		int charcode = m_DB.CreateCharactor(pUser->GetUserCode(), stParam.CharName, 100000000);
		if (charcode == -1)
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::SYSTEM_FAIL);
		}

		// ������ ĳ������ CharInfo ����
		std::string jstr = m_DB.GetCharInfoJsonStr(charcode);

		return SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS, jstr);
	}

	RESULTCODE HandleSelectChar(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		SelectCharParameter stParam;
		if (!m_JsonMaker.ToSelectCharParameter(param_, stParam))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM);
		}

		int usercode = pUser->GetUserCode();
		
		std::string strList;
		try
		{
			strList = m_DB.GetCharList(usercode);
		}
		catch (ToJsonException e)
		{
			std::cerr << "ReqHandler::HandleGetCharList : ToJsonException : " << e.what() << '\n';
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::UNDEFINED);
		}

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

		if (pInfo == nullptr)
		{
			std::cerr << "ReqHandler::HandleSelectChar : info nullptr\n";
		}

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

		// REQ�� ���� �㰡�� �� �Ŀ�
		RESULTCODE rescode = SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS);

		// map������ �Ѱܾ��Ѵ�. (map->UserEnter���� ������ �ִ� Info ����)
		pMap->UserEnter(userindex_, pUser);

		return rescode;
	}

	RESULTCODE HandleGetInven(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		GetInvenParameter stParam;
		if (!m_JsonMaker.ToGetInvenParameter(param_, stParam))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM);
		}
		
		std::string ret;

		if (!m_DB.GetInventory(stParam.charcode, ret))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::SYSTEM_FAIL);
		}

		return SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS, ret);
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
		if (!m_JsonMaker.ToGetObjectParameter(param_, stParam))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM);
		}

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

		// �κ��丮�� �߰�
		if (!m_DB.AddItem(pUser->GetCharCode(), itemobj->GetItemCode(),
			itemobj->GetExTime(), itemobj->GetCount()))
		{
			pMap->ReturnObject(itemobj);

			return SendResultMsg(userindex_, ReqNo, RESULTCODE::REQ_FAIL);
		}

		// ��ü ��ȯ
		m_MapManager.ReleaseItemObject(itemobj);

		return SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS);
	}

	RESULTCODE HandleGetSalesList(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		GetSalesListParameter stParam;

		if (!m_JsonMaker.ToGetSalesListParameter(param_, stParam))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM);
		}

		std::string res = m_DB.GetSalesList(stParam.npccode);

		// ���� ������ �־���.
		if (res.empty())
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::SYSTEM_FAIL);
		}

		return SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS, res);
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

		std::string strInven;

		if (!m_DB.GetInventory(pUser->GetCharCode(), strInven))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::SYSTEM_FAIL);
		}

		return SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS, strInven);
	}

	RESULTCODE HandleDropItem(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		DropItemParameter stParam;
		if (!m_JsonMaker.ToDropItemParameter(param_, stParam))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM);
		}

		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		if (stParam.mapcode != pUser->GetMapCode())
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::MODIFIED_MAPCODE);
		}

		std::string invenJstr;
		if (!m_DB.GetInventory(pUser->GetCharCode(), invenJstr))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::SYSTEM_FAIL);
		}

		Inventory inven;
		if (!m_JsonMaker.ToInventory(invenJstr, inven))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::UNDEFINED);
		}

		int nRet = m_DB.DropItem(pUser->GetCharCode(), stParam.slotidx, stParam.count);

		if (nRet == 0)
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::REQ_FAIL);
		}

		time_t exTime = inven[stParam.slotidx].expirationtime;

		RPG::Map* pMap = m_MapManager.GetMap(stParam.mapcode);

		//Vector2 position = pUser->GetPosition();

		Vector2 position(stParam.posx, stParam.posy);

		// ������Ʈ�� ������ ���� �����ϴ°� Map�̴�.
		pMap->CreateObject(nRet, exTime, stParam.count, 0, position);

		return SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS);
	}

	RESULTCODE HandleSwapInven(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		SwapInvenParameter stParam;
		if (!m_JsonMaker.ToSwapInvenParameter(param_, stParam))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM);
		}

		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		if (pUser->GetCharCode() == CLIENT_NOT_CERTIFIED)
		{
			// ���� ĳ���� ���õ� ���ߴµ�?
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_ORDER);
		}

		if (!m_DB.SwapInventory(pUser->GetCharCode(), stParam.idx1, stParam.idx2))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::SYSTEM_FAIL);
		}

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
		if (pMap == nullptr)
		{
			std::cerr << "ReqHandler::HandleMoveMap : Failed to Get Map pointer\n";
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::UNDEFINED);
		}

		pMap->UserExit(userindex_);
		pMap = m_MapManager.GetMap(stParam.tomapcode);

		if (pMap == nullptr)
		{
			std::cerr << "ReqHandler::HandleMoveMap : Failed to Get Map Pointer\n";
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::UNDEFINED);
		}

		if (!m_DB.MoveMap(pUser->GetCharCode(), stParam.tomapcode))
		{
			std::cerr << "ReqHandler::HandleMoveMap : Failed to MoveMap on Redis\n";
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::SYSTEM_FAIL);
		}

		// �ش� �ʿ� ó�� ������ ��� �ʱ�ȭ�� ���־���Ѵ�.
		if (pMap->IsNew())
		{
			// --�ʱ�ȭ
			// DB ��ȸ �� ���� ���� ����
			//pMap->InitSpawnInfo();

			// ��Ÿ ���� �ʱ�ȭ
			pMap->Init();
		}

		// REQ�� ���� �㰡�� �� �Ŀ�
		RESULTCODE rescode = SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS);

		// map������ �Ѱܾ��Ѵ�. (map->UserEnter���� ������ �ִ� Info ����)
		pMap->UserEnter(userindex_, pUser);

		return rescode;
	}

	RESULTCODE HandleSetGold(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);
		int charcode = pUser->GetCharCode();
		if (!m_DB.DebugSetGold(charcode))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::REQ_FAIL);
		}

		return SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS);
	}

	RESULTCODE HandleChatEveryone(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		RPG::Map* pMap = m_MapManager.GetMap(pUser->GetMapCode());

		if (pMap == nullptr)
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_ORDER);
		}

		// --�ش� �ʿ� ����

		std::ostringstream oss;

		std::string nickname = pUser->GetCharName();

		oss << "[" << nickname << "] : " + param_;

		std::string msg = oss.str();

		pMap->NotifyChatToAll(msg);

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
			std::cerr << "ReqHandler::SendResultMsg : Failed to Send\n";
			return RESULTCODE::UNDEFINED;
		}

		// �۽� ����
		return resCode_;
	}

	void SendInfoMsgToUsers(std::map<int, User*>& users, RESULTCODE rescode_, std::string& msg_, int exceptUsercode_ = 0)
	{
		if (users.size() == 0)
		{
			return;
		}

		ResMessage stResultMsg{ 0, rescode_, msg_ };
		std::string jsonmsg;
		if (!m_JsonMaker.ToJsonString(stResultMsg, jsonmsg))
		{
			std::cerr << "ReqHandler::SendInfoMsgToUsers : Failed to Make Jsonstr\n";
			return;
		}

		PacketData* packet = AllocatePacket();
		if (packet == nullptr)
		{
			std::cerr << "ReqHandler::SendInfoMsgToUsers : Failed to Allocate Packet\n";
			return;
		}

		auto first = users.begin();

		bool bExceptFirst = true;

		if (first->second == nullptr)
		{
			std::cerr << "ReqHandler::SendInfoMsgToUsers : User* null ref.\n";
		}
		else
		{
			bExceptFirst = (first->second->GetUserCode() == exceptUsercode_);
		}

		packet->Init(first->first, jsonmsg);

		for (auto& itr = ++first; itr != users.end(); itr++)
		{
			if (itr->second == nullptr)
			{
				std::cerr << "ReqHandler::SendInfoMsgToUsers : User* null ref.\n";
				continue;
			}

			if (itr->second->GetUserCode() == exceptUsercode_)
			{
				std::cout << "ReqHandler::SendInfoMsgToUsers : except usercode : " << itr->second->GetUserCode() << '\n';
				continue;
			}

			PacketData* tmpPacket = AllocatePacket();

			if (tmpPacket == nullptr)
			{
				DeallocatePacket(packet);
				std::cerr << "ReqHandler::SendInfoMsgToUsers : Failed to Allocate Packet\n";
				return;
			}

			tmpPacket->Init(packet, itr->first);
			if (!SendMsgFunc(tmpPacket))
			{
				// �۽� ����
				DeallocatePacket(packet);
				DeallocatePacket(tmpPacket);
				std::cerr << "ReqHandler::SendInfoMsgToUsers : Failed to Send Msg\n";
				return;
			}
		}

		if (bExceptFirst)
		{
			std::cout << "ReqHandler::SendInfoMsgToUsers : except usercode\n";
			return;
		}

		if (!SendMsgFunc(packet))
		{
			// �۽� ����
			DeallocatePacket(packet);
			std::cerr << "ReqHandler::SendInfoMsgToUsers : Failed to Send Msg\n";
		}

		return;
	}

	void SendInfoMsg(const int userindex_, RESULTCODE rescode_, std::string& msg_)
	{
		ResMessage stResultMsg{ 0, rescode_, msg_ };
		std::string jsonmsg;
		if (!m_JsonMaker.ToJsonString(stResultMsg, jsonmsg))
		{
			std::cerr << "ReqHandler::SendInfoMsg : Failed to Make Jsonstr\n";
			return;
		}

		PacketData* packet = AllocatePacket();
		if (packet == nullptr)
		{
			std::cerr << "ReqHandler::SendInfoMsg : Failed to Allocate Packet\n";
			return;
		}

		packet->Init(userindex_, jsonmsg);

		if (!SendMsgFunc(packet))
		{
			// �۽� ����
			DeallocatePacket(packet);
			std::cerr << "ReqHandler::SendInfoMsg : Failed to Send Msg\n";
		}

		return;
	}

	void RenewInfo(CharInfo& info_)
	{
		if (info_.CharNo == 0)
		{
			std::cerr << "ReqHandler::RenewInfo : CharNo Can't Be Zero.\n";
			return;
		}

		CharInfo* info = m_DB.GetCharInfo(info_.CharNo);

		info_ = *info;

		m_DB.ReleaseObject(info);

		return;
	}

	MapManager m_MapManager;
	JsonMaker m_JsonMaker;
	UserManager m_UserManager;
	Database m_DB;

	unsigned short m_MaxClient;

	typedef RESULTCODE(ReqHandler::* REQ_HANDLE_FUNC)(const int, const unsigned int, const std::string&);

	std::unordered_map<MessageType, REQ_HANDLE_FUNC> Actions;
};