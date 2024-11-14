#pragma once
#include <string>
#include "Database.hpp"
#include <unordered_map>
#include "MapManager.hpp"
#include "UserManager.hpp"

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

const float DISTANCE_LIMIT_GET_OBJECT = 5.0f;

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

		// ----- 인게임 -----
		Actions[MessageType::PERFORM_SKILL] = &ReqHandler::HandlePerformSkill;
		Actions[MessageType::GET_OBJECT] = &ReqHandler::HandleGetObject;
		Actions[MessageType::BUY_ITEM] = &ReqHandler::HandleBuyItem;
		Actions[MessageType::DROP_ITEM] = &ReqHandler::HandleDropItem;
		Actions[MessageType::USE_ITEM] = &ReqHandler::HandleUseItem;
		Actions[MessageType::MOVE_MAP] = &ReqHandler::HandleMoveMap;
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

	// 수행 결과로 의심스러운 동작이 검출된다면 따로 기록하는것도 좋겠다.
	void HandleReq(const int userindex_, std::string& Req_)
	{
		ReqMessage msg;
		if (!m_JsonMaker.ToReqMessage(Req_, msg))
		{
			// ReqMessage포맷조차 안맞췄다고..?
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
			if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM))
			{
				return RESULTCODE::UNDEFINED;
			}

			return RESULTCODE::WRONG_PARAM;
		}

		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		int nRet = m_DB.SignIn(stParam.id, stParam.pw, pUser->GetIP());

		// 정보 틀림
		if (nRet == CLIENT_NOT_CERTIFIED)
		{
			if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::SIGNIN_FAIL))
			{
				return RESULTCODE::UNDEFINED;
			}

			return RESULTCODE::SIGNIN_FAIL;
		}

		// 이미 로그인한 계정
		if (nRet == ALREADY_HAVE_SESSION)
		{
			if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::SIGNIN_ALREADY_HAVE_SESSION))
			{
				return RESULTCODE::UNDEFINED;
			}

			return RESULTCODE::SIGNIN_ALREADY_HAVE_SESSION;
		}

		pUser->SetUserCode(nRet);

		if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS))
		{
			return RESULTCODE::UNDEFINED;
		}

		return RESULTCODE::SUCCESS;
	}

	RESULTCODE HandleSignUp(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		SignUpParameter stParam;
		if (!m_JsonMaker.ToSignUpParameter(param_, stParam))
		{
			if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM))
			{
				return RESULTCODE::UNDEFINED;
			}

			return RESULTCODE::WRONG_PARAM;
		}

		eReturnCode eRet = m_DB.SignUp(stParam.id, stParam.pw, stParam.questno, stParam.ans, stParam.hint);

		switch (eRet)
		{
		case eReturnCode::SIGNUP_SUCCESS:
			if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS))
			{
				return RESULTCODE::UNDEFINED;
			}

			return RESULTCODE::SUCCESS;

		case eReturnCode::SIGNUP_ALREADY_IN_USE:
			if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::SIGNUP_ALREADY_IN_USE))
			{
				return RESULTCODE::UNDEFINED;
			}

			return RESULTCODE::SIGNUP_ALREADY_IN_USE;

		case eReturnCode::SIGNUP_FAIL:
			if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::SIGNUP_FAIL))
			{
				return RESULTCODE::UNDEFINED;
			}

			return RESULTCODE::SIGNUP_FAIL;

		case eReturnCode::SYSTEM_ERROR:
			if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::SYSTEM_FAIL))
			{
				return RESULTCODE::UNDEFINED;
			}

			return RESULTCODE::SYSTEM_FAIL;

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
			if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM))
			{
				return RESULTCODE::UNDEFINED;
			}

			return RESULTCODE::WRONG_PARAM;
		}

		if (usercode == CLIENT_NOT_CERTIFIED || usercode != stParam.usercode)
		{
			if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_ORDER))
			{
				return RESULTCODE::UNDEFINED;
			}

			return RESULTCODE::WRONG_ORDER;
		}

		std::string strCharList = m_DB.GetCharList(usercode);

		if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS, strCharList))
		{
			return RESULTCODE::UNDEFINED;
		}

		return RESULTCODE::SUCCESS;
	}

	RESULTCODE HandleGetCharInfo(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		GetCharInfoParameter stParam;
		if (!m_JsonMaker.ToGetCharInfoParameter(param_, stParam))
		{
			if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM))
			{
				return RESULTCODE::UNDEFINED;
			}

			return RESULTCODE::WRONG_PARAM;
		}
		
		std::string strCharInfo = m_DB.GetCharInfoJsonStr(stParam.charcode);

		if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS, strCharInfo))
		{
			return RESULTCODE::UNDEFINED;
		}

		return RESULTCODE::SUCCESS;
	}

	RESULTCODE HandleReserveCharName(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		ReserveCharNameParameter stParam;
		if (!m_JsonMaker.ToReserveCharNameParameter(param_, stParam))
		{
			if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM))
			{
				return RESULTCODE::UNDEFINED;
			}

			return RESULTCODE::WRONG_PARAM;
		}



		return RESULTCODE::SUCCESS;
	}

	RESULTCODE HandleCancelCharNameReserve(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		// 같은 파라미터를 사용한다.
		ReserveCharNameParameter stParam;
		if (!m_JsonMaker.ToReserveCharNameParameter(param_, stParam))
		{
			if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM))
			{
				return RESULTCODE::UNDEFINED;
			}

			return RESULTCODE::WRONG_PARAM;
		}



		return RESULTCODE::SUCCESS;
	}

	RESULTCODE HandleCreateChar(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		CreateCharParameter stParam;
		if (!m_JsonMaker.ToCreateCharParameter(param_, stParam))
		{
			if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM))
			{
				return RESULTCODE::UNDEFINED;
			}

			return RESULTCODE::WRONG_PARAM;
		}





		return RESULTCODE::SUCCESS;
	}

	RESULTCODE HandleSelectChar(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		SelectCharParameter stParam;
		if (!m_JsonMaker.ToSelectCharParameter(param_, stParam))
		{
			if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM))
			{
				return RESULTCODE::UNDEFINED;
			}

			return RESULTCODE::WRONG_PARAM;
		}

		std::string strList = m_DB.GetCharList(pUser->GetUserCode());
		CharList stCharList;
		if (!m_JsonMaker.ToCharList(strList, stCharList))
		{
			if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::SYSTEM_FAIL))
			{
				return RESULTCODE::UNDEFINED;
			}

			return RESULTCODE::SYSTEM_FAIL;
		}

		// 접속 권한 없음
		if (!stCharList.find(stParam.charcode))
		{
			if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_ORDER))
			{
				return RESULTCODE::UNDEFINED;
			}

			return RESULTCODE::WRONG_ORDER;
		}

		CharInfo* pInfo = m_DB.GetCharInfo(stParam.charcode);

		pUser->SetCharInfo(pInfo);

		RPG::Map* pMap = m_MapManager.GetMap(pInfo->LastMapCode);

		pMap->UserEnter(userindex_, pUser);

		if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS))
		{
			return RESULTCODE::UNDEFINED;
		}

		return RESULTCODE::SUCCESS;
	}

	// 클라이언트에서 자기 데미지는 임의의 값으로 판단해도 된다.
	// 임의판단이 맘에 안들면 시간변수를 이용해 난수를 이용한 시뮬레이션을 해도 될듯..
	RESULTCODE HandlePerformSkill(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		PerformSkillParameter stParam;
		if (!m_JsonMaker.ToPerformSkillParameter(param_, stParam))
		{
			if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM))
			{
				return RESULTCODE::UNDEFINED;
			}

			return RESULTCODE::WRONG_PARAM;
		}

		// --캐릭터가 스킬을 사용했다는 사실을 전파 (GameMap의 SendToAll을 만들고 사용하자.)
		
		// 적중한 몬스터 없음
		if (stParam.monsterbitmask == 0)
		{
			return RESULTCODE::SUCCESS;
		}

		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		// --시전한 스킬정보와 유저의 현재 스탯을 기반으로 데미지 계산


		// 해당 요청을 처리하기 전에 맵이 이동되어 반려.
		if (pUser->GetMapCode() != stParam.mapcode)
		{
			return RESULTCODE::MODIFIED_MAPCODE;
		}

		RPG::Map* pMap = m_MapManager.GetMap(pUser->GetMapCode());

		// --타격한 몬스터 정보에 맞게 맵에 전파 (맵에서 몬스터 데미지처리와 사망처리, 결과 전파 진행)

		return RESULTCODE::SUCCESS;
	}

	RESULTCODE HandleGetObject(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		GetObjectParameter stParam;
		m_JsonMaker.ToGetObjectParameter(param_, stParam);

		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		if (pUser->GetMapCode() != stParam.mapcode)
		{
			if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::MODIFIED_MAPCODE))
			{
				return RESULTCODE::UNDEFINED;
			}

			return RESULTCODE::MODIFIED_MAPCODE;
		}

		RPG::Map* pMap = m_MapManager.GetMap(pUser->GetMapCode());
		ItemObject* itemobj = pMap->PopObject(stParam.objectidx);

		// 이미 습득하였거나 유효기간이 지나 사라진 아이템
		if (itemobj == nullptr)
		{
			if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::REQ_FAIL))
			{
				return RESULTCODE::UNDEFINED;
			}

			return RESULTCODE::REQ_FAIL;
		}

		Vector2 position = itemobj->GetPosition();

		if (position.SquaredDistance(pUser->GetPosition()) <= DISTANCE_LIMIT_GET_OBJECT)
		{
			if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::REQ_FAIL))
			{
				return RESULTCODE::UNDEFINED;
			}

			return RESULTCODE::REQ_FAIL;
		}

		if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS))
		{
			return RESULTCODE::UNDEFINED;
		}

		// 객체 반환
		m_MapManager.ReleaseItemObject(itemobj);

		return RESULTCODE::SUCCESS;
	}

	RESULTCODE HandleBuyItem(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		BuyItemParameter stParam;
		if (!m_JsonMaker.ToBuyItemParameter(param_, stParam))
		{
			if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM))
			{
				return RESULTCODE::UNDEFINED;
			}

			return RESULTCODE::WRONG_PARAM;
		}

		// 1. 유저정보 확인
		
		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		// 2. 캐릭터 위치 확인
		
		RPG::Map* pMap = m_MapManager.GetMap(pUser->GetMapCode());
		if (pMap == nullptr)
		{
			if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_ORDER))
			{
				return RESULTCODE::UNDEFINED;
			}
			return RESULTCODE::WRONG_ORDER;
		}

		// 3. 해당 위치의 npc 확인
		


		// 4. npc의 판매목록 확인 : 유효기간도 확인할것.
	
		// 판매목록에 없음
		if (!m_DB.FindSalesList(stParam.npccode, stParam.itemcode))
		{
			if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_ORDER))
			{
				return RESULTCODE::UNDEFINED;
			}
			return RESULTCODE::WRONG_ORDER;
		}

		// 5. 아이템 구매 시도 (DB -> REDIS)

		// 소지금 부족 혹은 인벤토리 상태 때문에 실패
		if (!m_DB.BuyItem(pUser->GetCharCode(), stParam.itemcode, 0, stParam.count))
		{
			if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::REQ_FAIL))
			{
				return RESULTCODE::UNDEFINED;
			}

			return RESULTCODE::REQ_FAIL;
		}

		if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS))
		{
			return RESULTCODE::UNDEFINED;
		}

		return RESULTCODE::SUCCESS;
	}

	RESULTCODE HandleDropItem(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		DropItemParameter stParam;
		m_JsonMaker.ToDropItemParameter(param_, stParam);

		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);

		if (stParam.mapcode != pUser->GetMapCode())
		{
			if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::MODIFIED_MAPCODE))
			{
				return RESULTCODE::UNDEFINED;
			}

			return RESULTCODE::MODIFIED_MAPCODE;
		}

		int nRet = m_DB.DropItem(pUser->GetCharCode(), stParam.slotidx, stParam.count);

		if (nRet == 0)
		{
			if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::REQ_FAIL))
			{
				return RESULTCODE::UNDEFINED;
			}

			return RESULTCODE::REQ_FAIL;
		}

		RPG::Map* pMap = m_MapManager.GetMap(stParam.mapcode);

		Vector2 position = pUser->GetPosition();

		// 오브젝트가 생성된 것을 전파하는건 Map이다.
		pMap->CreateObject(nRet, stParam.count, 0, position);

		if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS))
		{
			return RESULTCODE::UNDEFINED;
		}

		return RESULTCODE::SUCCESS;
	}

	RESULTCODE HandleUseItem(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		// DB에 사용효과관련 테이블도 만들어야겠다.
		// HP회복 MP회복 STR증가 DEX증가 INT증가 MEN증가 PHYSICATT증가 MAGICATT증가 ...

		UseItemParameter stParam;

		if (!m_JsonMaker.ToUseItemParameter(param_, stParam))
		{
			if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM))
			{
				return RESULTCODE::UNDEFINED;
			}

			return RESULTCODE::WRONG_PARAM;
		}




		return RESULTCODE::SUCCESS;
	}

	RESULTCODE HandleMoveMap(const int userindex_, const unsigned int ReqNo, const std::string& param_)
	{
		MoveMapParameter stParam;
		if (!m_JsonMaker.ToMoveMapParameter(param_, stParam))
		{
			if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_PARAM))
			{
				return RESULTCODE::UNDEFINED;
			}

			return RESULTCODE::WRONG_PARAM;
		}

		User* pUser = m_UserManager.GetUserByConnIndex(userindex_);
		
		// 해당 맵에서 요청한 맵으로 갈 수 없음
		if (!m_DB.FindMapEdge(pUser->GetMapCode(), stParam.tomapcode))
		{
			if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::WRONG_ORDER))
			{
				return RESULTCODE::UNDEFINED;
			}

			return RESULTCODE::WRONG_ORDER;
		}

		// 맵 이동
		RPG::Map* pMap = m_MapManager.GetMap(pUser->GetMapCode());
		pMap->UserExit(userindex_);
		pMap = m_MapManager.GetMap(stParam.tomapcode);
		pMap->UserEnter(userindex_, pUser);

		if (!SendResultMsg(userindex_, ReqNo, RESULTCODE::SUCCESS))
		{
			return RESULTCODE::UNDEFINED;
		}

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

	bool SendResultMsg(const int userIndex_, const unsigned int ReqNo_,
		RESULTCODE resCode_, std::string& optionalMsg_)
	{
		ResMessage stResultMsg{ ReqNo_, resCode_, optionalMsg_ };
		std::string jsonmsg;
		if (!m_JsonMaker.ToJsonString(stResultMsg, jsonmsg))
		{
			std::cerr << "ReqHandler::SendResultMsg : Failed to Make Jsonstr\n";
			return false;
		}

		PacketData* packet = AllocatePacket();

		packet->Init(userIndex_, jsonmsg);

		return SendMsgFunc(packet);
	}
	
	bool SendResultMsg(const int userIndex_, const unsigned int ReqNo_,
		RESULTCODE resCode_, std::string&& optionalMsg_ = std::string())
	{
		ResMessage stResultMsg{ ReqNo_, resCode_, optionalMsg_ };
		std::string jsonmsg;
		if (!m_JsonMaker.ToJsonString(stResultMsg, jsonmsg))
		{
			std::cerr << "ReqHandler::SendResultMsg : Failed to Make Jsonstr\n";
			return false;
		}

		PacketData* packet = AllocatePacket();

		packet->Init(userIndex_, jsonmsg);

		return SendMsgFunc(packet);
	}

	MapManager m_MapManager;
	JsonMaker m_JsonMaker;
	UserManager m_UserManager;
	Database m_DB;

	typedef RESULTCODE(ReqHandler::* REQ_HANDLE_FUNC)(const int, const unsigned int, const std::string&);

	std::unordered_map<MessageType, REQ_HANDLE_FUNC> Actions;
};