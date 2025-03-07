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
* 각 동작으로 연결하는 건 hashmap을 이용해 연결한다. 
* 
* 각 동작을 핸들하는 함수에서 결과 송신까지 함께하기로 결정.
* 
* BuyItem은 일단 맵마다 NPC를 배치하고
* 1. 플레이어가 위치한 맵에 NPC가 있는지 확인하고 -> 해당 정보 DB에 추가 필요 [mapcode] [npccode]
* 2. NPC가 파는 물품에 해당 아이템이 있는지 확인하고 -> 해당 정보 DB에 추가 필요 [npccode] [itemcode]
* 3. 해당 아이템의 가격과 비교하여 구매할 수 있는지 (일단 DB를 통해 가격을 확인하고 Redis에 수정요청하는 부분은 만들어두었다.)
* 확인하는 방식으로 하면 좋을 것 같다.
* 아직 더 고민해보고 결정.
* 
* MoveMap은 DB에 맵간 연결정보를 기록하고 -> DB에 추가 필요 [mapcode] [mapcode]
* 이를 서버가 시작할때 받아와 DB가 가지고 있는 것으로 한다. (MapEdge)
* 이동가능여부를 DB로부터 확인받고 이행한다.
* 
* todo. DB에 npc-item 정보와 -> npc가 해당 아이템을 판매한다는 정보
* map-npc 정보와 -> 해당 맵에 npc가 존재한다는 정보
* item-price정보와 -> 해당 아이템의 기본 상점 구매가 정보
* map-map정보를 기록하여야함. -> 해당 맵에서 다음 맵으로 이동할 수 있다는 정보
*/


class ReqHandler
{
public:
	ReqHandler()
	{
		Actions = std::unordered_map<MessageType, REQ_HANDLE_FUNC>();

		// ----- 아웃게임 -----
		Actions[MessageType::SIGNIN] = &ReqHandler::HandleSignIn;
		Actions[MessageType::SIGNUP] = &ReqHandler::HandleSignUp;
		Actions[MessageType::MODIFY_PW] = &ReqHandler::HandleModifyPW;
		Actions[MessageType::GET_CHAR_LIST] = &ReqHandler::HandleGetCharList;
		Actions[MessageType::GET_CHAR_INFO] = &ReqHandler::HandleGetCharInfo; // 물론 이건 인게임에서 타유저 정보를 가져오는데도 써야한다.
		Actions[MessageType::RESERVE_CHAR_NAME] = &ReqHandler::HandleReserveCharName;
		Actions[MessageType::CANCEL_CHAR_NAME_RESERVE] = &ReqHandler::HandleCancelCharNameReserve;
		Actions[MessageType::CREATE_CHAR] = &ReqHandler::HandleCreateChar;
		Actions[MessageType::SELECT_CHAR] = &ReqHandler::HandleSelectChar;
		Actions[MessageType::GET_INVEN] = &ReqHandler::HandleGetInven;
		Actions[MessageType::GET_SALESLIST] = &ReqHandler::HandleGetSalesList;
		Actions[MessageType::SWAP_INVENTORY] = &ReqHandler::HandleSwapInven;

		// ----- 인게임 -----
		Actions[MessageType::PERFORM_SKILL] = &ReqHandler::HandlePerformSkill;
		Actions[MessageType::GET_OBJECT] = &ReqHandler::HandleGetObject;
		Actions[MessageType::BUY_ITEM] = &ReqHandler::HandleBuyItem;
		Actions[MessageType::DROP_ITEM] = &ReqHandler::HandleDropItem;
		Actions[MessageType::USE_ITEM] = &ReqHandler::HandleUseItem;
		Actions[MessageType::MOVE_MAP] = &ReqHandler::HandleMoveMap;
		Actions[MessageType::CHAT_EVERYONE] = &ReqHandler::HandleChatEveryone;
		Actions[MessageType::POS_INFO] = &ReqHandler::HandlePosInfo;

		// ----- 디버깅 테스트용 (배포시 제거 후 배포) -----
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

	// 수행 결과로 의심스러운 동작이 검출된다면 따로 기록하는것도 좋겠다.
	bool HandleReq(const int userindex_, std::string& Req_)
	{
		ReqMessage msg;
		if (!m_JsonMaker.ToReqMessage(Req_, msg))
		{
			std::cout << "ReqHandler::HandleReq : ToReqMsg 실패\n";
			// ReqMessage포맷조차 안맞췄다고..?
			return false;
		}

		auto itr = Actions.find(msg.type);

		if (itr == Actions.end())
		{
			std::cerr << "ReqHandler::HandleReq : Not Defined ReqType.\n";
			return false;
		}

		// 함수포인터를 통한 사용
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

		// 변동사항을 기록하고 객체를 반환한다.
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

		// 정보 틀림
		if (nRet == CLIENT_NOT_CERTIFIED)
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::SIGNIN_FAIL);
		}

		// 이미 로그인한 계정
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
		// Database 쪽 함수 먼저 만들 것

		return RESULTCODE::SUCCESS;
	}

	/// <summary>
	/// SignIn이랑 독립된 구조로 만들려고 했는데
	/// SignIn 후에 어차피 CharList를 요청하여야한다.
	/// SignIn 동작 내에도 구현되어 있다.
	/// GetCharList만 독립적으로 사용할지는 아직 모르겠음.
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
		// 같은 파라미터를 사용한다.
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

		// 예약이 만료 혹은 잘못된 요청
		// 혹시 여기 분기에 걸릴지도 모르니 서버에서 판단하는 시간보다 클라이언트에서 판단하는 유효기간을 짧게 할 것.
		if (pUser->GetReserveNickname() != stParam.CharName)
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::REQ_FAIL);
		}

		// 시작맵 코드를 아직 안정했다. 여러맵에서 시작하게 할까 아니면 한 맵에서만 시작할까
		int charcode = m_DB.CreateCharactor(pUser->GetUserCode(), stParam.CharName, 100000000);
		if (charcode == -1)
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::SYSTEM_FAIL);
		}

		// 생성한 캐릭터의 CharInfo 전달
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

		// 접속 권한 없음
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

		// 맵 생성 직후 초기화 필요
		if (pMap->IsNew())
		{
			std::vector<MonsterSpawnInfo> spawnInfo = m_DB.GetMonsterSpawnInfo(pInfo->LastMapCode);

			for (size_t i = 0; i < spawnInfo.size(); i++)
			{
				pMap->InitSpawnInfo(i, spawnInfo[i]);
			}
		}

		// REQ에 대해 허가를 한 후에
		RESULTCODE rescode = SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS);

		// map정보를 넘겨야한다. (map->UserEnter에서 가지고 있는 Info 전파)
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

	// 클라이언트에서 자기 데미지는 시간변수를 이용해 난수를 이용한 시뮬레이션을 해도 될듯..
	RESULTCODE HandlePerformSkill(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		PerformSkillParameter stParam;
		if (!m_JsonMaker.ToPerformSkillParameter(param_, stParam))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM);
		}

		// --캐릭터가 스킬을 사용했다는 사실을 전파 (GameMap의 SendToAll을 만들고 사용하자.)

		// 적중한 몬스터 없음
		if (stParam.monsterbitmask == 0)
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS);
		}

		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		// 일단 기준값을 계산하고, 몬스터에 적용할 때 난수를 곱하자.
		long long damage = pUser->calStatDamage(); 

		// 해당 요청을 처리하기 전에 맵이 이동되어 반려.
		if (pUser->GetMapCode() != stParam.mapcode)
		{
			return RESULTCODE::MODIFIED_MAPCODE;
		}

		RPG::Map* pMap = m_MapManager.GetMap(pUser->GetMapCode());

		stParam.monsterbitmask;

		for (int bit = 1, cnt = 0; cnt < RPG::MAX_MONSTER_COUNT_ON_MAP; cnt++, bit *= 2)
		{
			// 해당 비트의 몬스터는 타격대상이 아님.
			if ((stParam.monsterbitmask & bit) == 0)
			{
				continue;
			}

			// 일단 0.75 ~ 1.25의 값을 곱해서 최종적으로 데미지를 정한다.
			long long finalDamage = ChanceEvaluator::GetInstance().CalDamage(damage, 0.25);

			if (finalDamage == 0)
			{
				continue;
			}

			auto pair = pMap->AttackMonster(userindex_, cnt, finalDamage);

			if (pair.first != 0)
			{
				// --해당 몬스터에 데미지를 주었음을 전파
			}

			if (pair.second != 0)
			{
				// --해당 코드에 맞는 아이템 생성
				// --해당 맵에 사망처리 전파
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

		// 이미 습득하였거나 사라졌거나 거리가 멀거나 습득권한이 없는 아이템
		if (itemobj == nullptr)
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::REQ_FAIL);
		}

		// 인벤토리에 추가
		if (!m_DB.AddItem(pUser->GetCharCode(), itemobj->GetItemCode(),
			itemobj->GetExTime(), itemobj->GetCount()))
		{
			pMap->ReturnObject(itemobj);

			return SendResultMsg(userindex_, ReqNo, RESULTCODE::REQ_FAIL);
		}

		// 객체 반환
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

		// 무언가 오류가 있었다.
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

		// 1. 유저정보 확인
		
		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		// 2. 캐릭터 위치 확인
		
		RPG::Map* pMap = m_MapManager.GetMap(pUser->GetMapCode());
		if (pMap == nullptr)
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_ORDER);
		}

		// 3. 해당 위치의 npc 확인
		
		if (!m_DB.FindNPCInfo(pUser->GetMapCode(), stParam.npccode))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_ORDER);
		}

		// 4. npc의 판매목록 확인 : 유효기간도 확인할것.
	
		// 판매목록에 없음
		if (!m_DB.FindSalesList(stParam.npccode, stParam.itemcode))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_ORDER);
		}

		// 5. 아이템 구매 시도 (DB -> REDIS)

		// 소지금 부족 혹은 인벤토리 상태 때문에 실패
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

		// 오브젝트가 생성된 것을 전파하는건 Map이다.
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
			// 아직 캐릭터 선택도 안했는데?
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
		// DB에 사용효과관련 테이블도 만들어야겠다.
		// HP회복 MP회복 STR증가 DEX증가 INT증가 MEN증가 PHYSICATT증가 MAGICATT증가 ...

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
		
		// 해당 맵에서 요청한 맵으로 갈 수 없음
		if (!m_DB.FindMapEdge(pUser->GetMapCode(), stParam.tomapcode))
		{
			return SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_ORDER);
		}

		// 맵 이동
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

		// 해당 맵에 처음 들어오는 경우 초기화를 해주어야한다.
		if (pMap->IsNew())
		{
			// --초기화
			// DB 조회 및 스폰 정보 적용
			//pMap->InitSpawnInfo();

			// 기타 정보 초기화
			pMap->Init();
		}

		// REQ에 대해 허가를 한 후에
		RESULTCODE rescode = SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS);

		// map정보를 넘겨야한다. (map->UserEnter에서 가지고 있는 Info 전파)
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

		// --해당 맵에 전파

		std::ostringstream oss;

		std::string nickname = pUser->GetCharName();

		oss << "[" << nickname << "] : " + param_;

		std::string msg = oss.str();

		pMap->NotifyChatToAll(msg);

		return RESULTCODE::SUCCESS;
	}

	// 해당 처리는 클라이언트에게 결과반환을 하지 않는다.
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
			// 송신 실패
			return RESULTCODE::UNDEFINED;
		}

		// 송신 성공
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
			// 송신 실패
			std::cerr << "ReqHandler::SendResultMsg : Failed to Send\n";
			return RESULTCODE::UNDEFINED;
		}

		// 송신 성공
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
				// 송신 실패
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
			// 송신 실패
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
			// 송신 실패
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