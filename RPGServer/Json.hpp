#pragma once

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"
#include <string>
#include "CharInfo.hpp"
#include "CharList.hpp"
#include "Inventory.hpp"
#include <codecvt>
#include <iostream>

/*
* wstring을 codecvt한 것과 string은 서로 다른 값을 가진다.
* -> 인코딩이 서로 다르기 때문.
* 문자열은 string으로만 입력받는게 나을듯.
* 
* 구조체->json : struct -> string
* json->구조체 : string -> struct
* 
* 캐릭터 닉네임 예약
* 1. 캐릭터 생성 과정 중 닉네임 중복확인 후 사용할지 선택하는 과정을 거친다.
* 2. 사용할지 선택하는 과정 중엔 예약된 상태로 다른 유저가 접근할 수 없도록 한다.
*  예약 : Redis에 락을 거는데 성공했다면 User 정보에 획득한 락을 기록할 수 있도록 한다.
*  예약 이후 1분의 시간을 부여하여 선택한다. 1분 내에 선택하지 않으면 락을 해제한다. 락의 유효기간은 2분으로 설정한다.
*  - 사용한다 선택 (캐릭터 생성 요청)
*	예약이 해당 유저에게 걸려있는지 User를 통해 확인하고 락이 있다면 캐릭터를 생성
*  - 사용하지 않는다 선택
*   해당 유저가 락이 있다면 예약을 해제한다.
*/ 


enum class MessageType
{
	SIGNIN,
	SIGNUP,
	MODIFY_PW,
	GET_CHAR_LIST,
	GET_CHAR_INFO,
	RESERVE_CHAR_NAME, // 캐릭터 생성 시 닉네임을 입력할 경우 미리 예약을 건 후, 가능한 닉네임인 경우 사용할 지 여부를 선택한다.
	CANCEL_CHAR_NAME_RESERVE, // RESERVE_CHAR_NAME으로 예약한 닉네임을 취소한다.
	CREATE_CHAR, // 캐릭터 생성 결정. DB에 캐릭터 정보를 기록한다.
	SELECT_CHAR,
	PERFORM_SKILL,
	GET_OBJECT,
	BUY_ITEM,
	DROP_ITEM,
	USE_ITEM,
	MOVE_MAP, // 맵 이동 요청
	CHAT_EVERYONE, // 해당 맵의 모든 인원에게 채팅 (다른 종류의 채팅은 더 생각해보자.)

	POS_INFO, // 캐릭터의 위치, 속도 등에 대한 정보를 업데이트하는 파라미터
	LAST = POS_INFO // enum class가 수정되면 마지막 원소로 지정할 것
};

enum class RESULTCODE
{
	SUCCESS,
	WRONG_PARAM,					// 요청번호와 맞지 않는 파라미터
	SYSTEM_FAIL,					// 시스템의 문제
	SIGNIN_FAIL,					// ID와 PW가 맞지 않음
	SIGNIN_ALREADY_HAVE_SESSION,	// 이미 로그인된 계정
	SIGNUP_FAIL,					// ID규칙이나 PW규칙이 맞지 않음
	SIGNUP_ALREADY_IN_USE,			// 이미 사용중인 ID
	WRONG_ORDER,					// 요청이 상황에 맞지 않음
	MODIFIED_MAPCODE,				// 해당 요청이 들어올 때의 맵코드와 현재 서버가 가지고 있는 해당 유저의 맵코드가 상이함.
	REQ_FAIL,						// 현재 조건에 맞지 않아 실행이 실패함 (이미 사라진 아이템 등..)
	SEND_INFO,						// 해당 유저의 요청에 의해 전달된 정보가 아닌 정보전달 (채팅, 맵변화, 다른플레이어 상호작용...)
	UNDEFINED						// 알수없는 오류
};

struct ReqMessage
{
	MessageType type;
	unsigned int reqNo; // for response. res of reqNo.
	std::string msg; // parameter of the type to json
};

struct ResMessage
{
	unsigned int reqNo; // which msg's result
	RESULTCODE resCode; // result (success, fail ...)
	std::string msg; // optional (ex. charinfo jsonstr)
};

struct SignInParameter
{
	std::string id;
	std::string pw;
};

struct SignUpParameter
{
	std::string id;
	std::string pw;
	int questno; // 비밀번호 변경 문제
	std::string ans; // 비밀번호 변경 답
	std::string hint; // 비밀번호 변경 문제 힌트(유저기입)
};

struct ModifyPWParameter
{
	std::string id;
	std::string ans;
	std::string newpw;
};

struct GetCharListParameter
{
	int usercode;
};

struct GetCharInfoParameter
{
	int charcode;
};

struct ReserveCharNameParameter
{
	std::string CharName;
};

struct CreateCharParameter
{
	std::string CharName;
	// 캐릭터만 만들고 모든 진행이 가능하게 할지
	// 계통을 선택하고 캐릭터를 만들면 해당 계통 내에서 가능하게 할지 고민 필요
};

struct SelectCharParameter
{
	int charcode;
};

struct PerformSkillParameter
{
	int mapcode;
	int monsterbitmask; // 1011 : 0번, 1번, 3번 몬스터에게 타격, 32bit니까 한 맵의 몬스터는 32마리까지 됨
	int skillcode;
	long long timeValue; // 클라이언트에서 측정한 시간, 해당 시간을 기반으로 난수사용하여 데미지 계산
};

struct GetObjectParameter
{
	int mapcode;
	int objectidx;
};

struct BuyItemParameter
{
	int npccode;
	int itemcode;
	int count;
};

struct DropItemParameter
{
	int mapcode;
	int slotidx;
	int count;
};

struct UseItemParameter
{
	int itemidx;
};

struct MoveMapParameter
{
	int tomapcode;
};

struct ChatEveryoneParameter
{

};

struct PosInfoParameter
{
	Vector2 pos;
	Vector2 vel;
};

class JsonMaker
{
public:
	bool ToJsonString(const CharInfo& pInfo_, std::string& out_)
	{
		rapidjson::Document doc;
		doc.SetObject();

		rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

		// structType
		doc.AddMember("Type", "CharInfo", allocator);

		// int Members
		doc.AddMember("CharNo", pInfo_.CharNo, allocator);
		doc.AddMember("Level", pInfo_.Level, allocator);
		doc.AddMember("Experience", pInfo_.Experience, allocator);
		doc.AddMember("StatPoint", pInfo_.StatPoint, allocator);
		doc.AddMember("HealthPoint", pInfo_.HealthPoint, allocator);
		doc.AddMember("ManaPoint", pInfo_.ManaPoint, allocator);
		doc.AddMember("Strength", pInfo_.Strength, allocator);
		doc.AddMember("Dexterity", pInfo_.Dexterity, allocator);
		doc.AddMember("Intelligence", pInfo_.Intelligence, allocator);
		doc.AddMember("Mentality", pInfo_.Mentality, allocator);
		doc.AddMember("Gold", pInfo_.Gold, allocator);
		doc.AddMember("LastMapCode", pInfo_.LastMapCode, allocator);

		// wchar_t[] Member
		std::string utf8str = m_cvt.to_bytes(pInfo_.CharName);
		doc.AddMember("CharName", rapidjson::Value().SetString(utf8str.c_str(), static_cast<rapidjson::SizeType>(utf8str.length())), allocator);

		// Make JsonString
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		doc.Accept(writer);

		out_ = buffer.GetString();

		return true;
	}

	bool ToCharInfo(const std::string& jsonStr_, CharInfo& out_)
	{
		rapidjson::Document doc;
		doc.Parse(jsonStr_.c_str());

		if (doc.HasMember("Type") && doc["Type"].IsString() && strcmp(doc["Type"].GetString(), "CharInfo") == 0)
		{
			// wstring Member
			const char* str = doc["CharName"].GetString();
			std::wstring wstr = m_cvt.from_bytes(str).c_str();

			if (wstr.length() >= 11)
			{
				return false;
			}
			wcscpy_s(out_.CharName, wstr.c_str());

			// int Members
			out_.CharNo = doc["CharNo"].GetInt();
			out_.Level = doc["Level"].GetInt();
			out_.Experience = doc["Experience"].GetInt();
			out_.StatPoint = doc["StatPoint"].GetInt();
			out_.HealthPoint = doc["StatPoint"].GetInt();
			out_.ManaPoint = doc["ManaPoint"].GetInt();
			out_.Strength = doc["Strength"].GetInt();
			out_.Dexterity = doc["Dexterity"].GetInt();
			out_.Intelligence = doc["Intelligence"].GetInt();
			out_.Mentality = doc["Mentality"].GetInt();
			out_.Gold = doc["Gold"].GetInt();
			out_.LastMapCode = doc["LastMapCode"].GetInt();
		}
		else
		{
			return false;
		}
	}

	std::string ToJsonString(CharList& charlist_)
	{
		rapidjson::Document doc;
		doc.SetObject();

		auto& allocator = doc.GetAllocator();

		doc.AddMember("Type", "CharList", allocator);

		rapidjson::Value charcodeList(rapidjson::kArrayType);

		for (size_t i = 0; i < charlist_.size(); i++)
		{
			charcodeList.PushBack(charlist_[i], allocator);
		}

		doc.AddMember("CharList", charcodeList, allocator);

		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		doc.Accept(writer);

		return buffer.GetString();
	}

	// out_ 매개변수로 들어온 객체는 초기화된다. 주의.
	bool ToCharList(const std::string& str_, CharList& out_)
	{
		rapidjson::Document doc;
		if (doc.Parse(str_.c_str()).HasParseError())
		{
			std::cerr << "Json::ToCharList : " << rapidjson::GetParseError_En(doc.GetParseError()) << '\n';
			return false;
		}

		if (!doc.HasMember("Type") || !doc["Type"].IsString() || strcmp(doc["Type"].GetString(), "CharList") != 0)
		{
			std::cerr << "Json::ToCharList : Not CharList JsonStr.\n";
			return false;
		}

		if (!doc.HasMember("CharList") || !doc["CharList"].IsArray())
		{
			std::cerr << "Json::ToCharList : InCorrect Format.\n";
			return false;
		}

		rapidjson::Value& array = doc["CharList"];
		out_.Init(array.Size()); // 초기화

		for (size_t i = 0; i < out_.size(); i++)
		{
			if (!array[i].IsInt())
			{
				std::cerr << "Json::ToCharList : Not Int Variable.\n";
				return false;
			}
			out_[i] = array[i].GetInt();
		}

		return true;
	}

	// set of Item
	// slot < MAX_SLOT
	// Item : int itemcode, long long expirationtime, int count
	bool ToJsonString(Inventory& inven_, std::string& out_)
	{
		rapidjson::Document doc;
		doc.SetObject();

		auto& allocator = doc.GetAllocator();

		rapidjson::Value array(rapidjson::kArrayType);

		for (size_t i = 0; i < MAX_SLOT; i++)
		{
			rapidjson::Value itemValue(rapidjson::kObjectType);

			const Item& item = inven_[i];

			itemValue.AddMember("ItemCode", inven_[i].itemcode, allocator);
			itemValue.AddMember("ExTime", inven_[i].expirationtime, allocator);
			itemValue.AddMember("Count", inven_[i].count, allocator);
			array.PushBack(itemValue, allocator);
		}
		
		doc.AddMember("Inven", array, allocator);

		// Make JsonString
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		doc.Accept(writer);

		out_ = buffer.GetString();

		return true;
	}

	bool ToJsonString(const ResMessage& res_, std::string& out_)
	{
		rapidjson::Document doc;
		doc.SetObject();

		auto& allocator = doc.GetAllocator();

		doc.AddMember("ReqNo", res_.reqNo, allocator);
		doc.AddMember("ResCode", static_cast<int>(res_.resCode), allocator);

		// optional
		if (!res_.msg.empty())
		{
			// rapidjson::Document::AddMember에서 std::string을 호환하지 않는다.
			// rapidjson::Value로 변환하여 전달한다.
			
			rapidjson::Value val;
			val.SetString(res_.msg.c_str(), allocator);
			doc.AddMember("Msg", val, allocator);
		}

		// Make JsonString
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		doc.Accept(writer);

		out_ = buffer.GetString();

		return true;
	}

	// out_ 매개변수로 들어온 객체는 초기화된다. 주의.
	bool ToInventory(const std::string& str_, Inventory& out_)
	{
		rapidjson::Document doc;

		if (doc.Parse(str_.c_str()).HasParseError())
		{
			std::cerr << "Json::ToInventory : " << rapidjson::GetParseError_En(doc.GetParseError()) << '\n';
			return false;
		}

		if (!doc.HasMember("Inven") || !doc["Inven"].IsArray())
		{
			std::cerr << "Json::ToInventory : Incorrect Format.\n";
			return false;
		}

		const rapidjson::Value& Inven = doc["Inven"];

		for (size_t i = 0; i < MAX_SLOT; i++)
		{
			if (i >= Inven.Size())
			{
				out_[i].Init();
				continue;
			}

			if (!Inven[i].HasMember("ItemCode") || !Inven[i]["ItemCode"].IsInt() ||
				!Inven[i].HasMember("ExTime") || !Inven[i]["ExTime"].IsInt64() ||
				!Inven[i].HasMember("Count") || !Inven[i]["Count"].IsInt())
			{
				std::cerr << "Json::ToInventory : Incorrect Format.\n";
				return false;
			}

			out_[i].Init(Inven[i]["ItemCode"].GetInt(), Inven[i]["ExTime"].GetInt64(), Inven[i]["Count"].GetInt());
		}

		return true;
	}

	bool ToReqMessage(const std::string& str_, ReqMessage& out_)
	{
		rapidjson::Document doc;
		if (!doc.Parse(str_.c_str()).HasParseError())
		{
			std::cerr << "Json::ToReqMessage : " << rapidjson::GetParseError_En(doc.GetParseError()) << "\n";
			return false;
		}

		if (!doc.HasMember("Type") || !doc["Type"].IsInt() ||
			!doc.HasMember("ReqNo") || !doc["ReqNo"].IsInt() ||
			!doc.HasMember("msg") || !doc["msg"].IsString())
		{
			std::cerr << "Json::ToReqMessage : Incorrect Format.\n";
			return false;
		}

		int type = doc["Type"].GetInt();

		if (type > static_cast<int>(MessageType::LAST) || type < static_cast<int>(MessageType::SIGNIN))
		{
			std::cerr << "Json::ToReqMessage : Not Defined MessageType.\n";
			return false;
		}

		out_.type = static_cast<MessageType>(type);
		out_.reqNo = doc["ReqNo"].GetInt();
		out_.msg = std::string(doc["msg"].GetString());

		return true;
	}

	bool ToSignInParameter(const std::string& str_, SignInParameter& out_)
	{
		rapidjson::Document doc;
		if (doc.Parse(str_.c_str()).HasParseError())
		{
			std::cerr << "Json::ToSignInParam : " << rapidjson::GetParseError_En(doc.GetParseError()) << "\n";
			return false;
		}

		if (!doc.HasMember("ID") || !doc["ID"].IsString() ||
			!doc.HasMember("PW") || !doc["PW"].IsString())
		{
			std::cerr << "Json::ToSignInParam : Incorrect Format.\n";
			return false;
		}

		out_.id = doc["ID"].GetString();
		out_.pw = doc["PW"].GetString();

		return true;
	}

	bool ToSignUpParameter(const std::string& str_, SignUpParameter& out_)
	{
		rapidjson::Document doc;
		if (doc.Parse(str_.c_str()).HasParseError())
		{
			std::cerr << "Json::ToSignUpParam : " << rapidjson::GetParseError_En(doc.GetParseError()) << "\n";
			return false;
		}

		if (!doc.HasMember("ID") || !doc["ID"].IsString() ||
			!doc.HasMember("PW") || !doc["PW"].IsString() ||
			!doc.HasMember("QuestNo") || !doc["QuestNo"].IsInt() ||
			!doc.HasMember("Answer") || !doc["Answer"].IsString() ||
			!doc.HasMember("Hint") || !doc["Hint"].IsString())
		{
			std::cerr << "Json::ToSignUpParam : Incorrect Format.\n";
			return false;
		}

		out_.id = doc["ID"].GetString();
		out_.pw = doc["PW"].GetString();
		out_.questno = doc["QuestNo"].GetInt();
		out_.ans = doc["Answer"].GetString();
		out_.hint = doc["Hint"].GetString();

		return true;
	}

	bool ToModifyPWParameter(const std::string& str_, ModifyPWParameter& out_)
	{
		rapidjson::Document doc;
		if (doc.Parse(str_.c_str()).HasParseError())
		{
			std::cerr << "Json::ToModifyPWParam : " << rapidjson::GetParseError_En(doc.GetParseError()) << "\n";
			return false;
		}

		if (!doc.HasMember("ID") || !doc["ID"].IsString() ||
			!doc.HasMember("Answer") || !doc["Answer"].IsString())
		{
			std::cerr << "Json::ToModifyPWParam : Incorrect Format.\n";
			return false;
		}

		out_.id = doc["ID"].GetString();
		out_.ans = doc["Answer"].GetString();

		return true;
	}

	bool ToGetCharListParameter(const std::string& str_, GetCharListParameter& out_)
	{
		rapidjson::Document doc;
		if (doc.Parse(str_.c_str()).HasParseError())
		{
			std::cerr << "Json::ToGetCharListParam : " << rapidjson::GetParseError_En(doc.GetParseError()) << "\n";
			return false;
		}

		if (!doc.HasMember("Usercode") || !doc["Usercode"].IsInt())
		{
			std::cerr << "Json::ToGetCharListParam : Incorrect Format.\n";
			return false;
		}

		out_.usercode = doc["Usercode"].GetInt();

		return true;
	}

	bool ToGetCharInfoParameter(const std::string& str_, GetCharInfoParameter& out_)
	{
		rapidjson::Document doc;
		if (doc.Parse(str_.c_str()).HasParseError())
		{
			std::cerr << "Json::ToGetCharInfoParam : " << rapidjson::GetParseError_En(doc.GetParseError()) << "\n";
			return false;
		}

		if (!doc.HasMember("Charcode") || !doc["Charcode"].IsInt())
		{
			std::cerr << "Json::ToGetCharInfoParam : Incorrect Format.\n";
			return false;
		}

		out_.charcode = doc["Charcode"].GetInt();

		return true;
	}

	// CancelReserve와 동일한 parameter를 사용한다.
	bool ToReserveCharNameParameter(const std::string& str_, ReserveCharNameParameter& out_)
	{
		rapidjson::Document doc;
		if (doc.Parse(str_.c_str()).HasParseError())
		{
			std::cerr << "Json::ToReserveCharNameParam : " << rapidjson::GetParseError_En(doc.GetParseError()) << '\n';
			return false;
		}

		if (!doc.HasMember("CharName") || !doc["CharName"].IsString())
		{
			std::cerr << "Json::ToReserveCharNameParam : Incorrect Format.\n";
			return false;
		}

		out_.CharName = std::string(doc["CharName"].GetString());

		return true;
	}

	bool ToCreateCharParameter(const std::string& str_, CreateCharParameter& out_)
	{




		return true;
	}

	bool ToSelectCharParameter(const std::string& str_, SelectCharParameter& out_)
	{
		rapidjson::Document doc;
		if (doc.Parse(str_.c_str()).HasParseError())
		{
			std::cerr << "Json::ToSelectCharParam : " << rapidjson::GetParseError_En(doc.GetParseError()) << "\n";
			return false;
		}

		if (!doc.HasMember("Charcode") || !doc["Charcode"].IsInt())
		{
			std::cerr << "Json::ToSelectCharParam : Incorrect Format.\n";
			return false;
		}

		out_.charcode = doc["Charcode"].GetInt();

		return true;
	}

	bool ToPerformSkillParameter(const std::string& str_, PerformSkillParameter& out_)
	{
		rapidjson::Document doc;
		if (doc.Parse(str_.c_str()).HasParseError())
		{
			std::cerr << "Json::ToPerformSkillParam : " << rapidjson::GetParseError_En(doc.GetParseError()) << "\n";
			return false;
		}

		if (!doc.HasMember("MapCode") || !doc["MapCode"].IsInt() ||
			!doc.HasMember("Target") || !doc["Target"].IsInt() ||
			!doc.HasMember("SkillCode") || !doc["SkillCode"].IsInt() ||
			!doc.HasMember("Time") || !doc["Time"].IsInt64())
		{
			std::cerr << "Json::ToPerformSkillParam : Incorrect Format.\n";
			return false;
		}

		out_.mapcode = doc["MapCode"].GetInt();
		out_.monsterbitmask = doc["Target"].GetInt();
		out_.skillcode = doc["SkillCode"].GetInt();
		out_.timeValue = doc["Time"].GetInt64();

		return true;
	}

	bool ToBuyItemParameter(const std::string& str_, BuyItemParameter& out_)
	{
		rapidjson::Document doc;

		if (doc.Parse(str_.c_str()).HasParseError())
		{
			std::cerr << "Json::ToBuyItemParam : " << rapidjson::GetParseError_En(doc.GetParseError()) << '\n';
			return false;
		}

		if (!doc.HasMember("Count") || !doc["Count"].IsInt() ||
			!doc.HasMember("ItemCode") || !doc["ItemCode"].IsInt() ||
			!doc.HasMember("NPCCode") || !doc["NPCCode"].IsInt())
		{
			std::cerr << "Json::ToBuyItemParam : Incorrect Format\n";
			return false;
		}

		out_.count = doc["Count"].GetInt();
		out_.itemcode = doc["ItemCode"].GetInt();
		out_.npccode = doc["NPCCode"].GetInt();

		return true;
	}

	bool ToGetObjectParameter(const std::string& str_, GetObjectParameter& out_)
	{
		rapidjson::Document doc;

		if (doc.Parse(str_.c_str()).HasParseError())
		{
			std::cerr << "Json::ToGetObjectParam : " << rapidjson::GetParseError_En(doc.GetParseError()) << "\n";
			return false;
		}

		if (!doc.HasMember("MapCode") || !doc["MapCode"].IsInt() ||
			!doc.HasMember("ItemIdx") || !doc["ItemIdx"].IsUint())
		{
			std::cerr << "Json::ToGetObjectParam : Incorrect Format.\n";
			return false;
		}
		
		out_.mapcode = doc["MapCode"].GetInt();
		out_.objectidx = doc["ItemIdx"].GetUint();

		return true;
	}

	bool ToDropItemParameter(const std::string& str_, DropItemParameter& out_)
	{
		rapidjson::Document doc;
		if (!doc.Parse(str_.c_str()).HasParseError())
		{
			std::cerr << "Json::ToDropItemParam : " << rapidjson::GetParseError_En(doc.GetParseError()) << "\n";
			return false;
		}

		if (!doc.HasMember("MapCode") || !doc["MapCode"].IsInt() ||
			!doc.HasMember("SlotIdx") || !doc["SlotIdx"].IsInt() ||
			!doc.HasMember("Count") || !doc["Count"].IsInt())
		{
			std::cerr << "Json::ToDropItemParam : Incorrect Format.\n";
			return false;
		}

		out_.mapcode = doc["MapCode"].GetInt();
		out_.slotidx = doc["SlotIdx"].GetInt();
		out_.count = doc["Count"].GetInt();

		return true;
	}

	bool ToUseItemParameter(const std::string& str_, UseItemParameter& out_)
	{
		rapidjson::Document doc;
		if (!doc.Parse(str_.c_str()).HasParseError())
		{
			std::cerr << "Json::ToUseItemParam : " << rapidjson::GetParseError_En(doc.GetParseError()) << "\n";
			return false;
		}

		if (!doc.HasMember("ItemIdx") || !doc["ItemIdx"].IsInt())
		{
			std::cerr << "Json::ToUseItemParam : Incorrect Format\n";

			return false;
		}

		out_.itemidx = doc["ItemIdx"].GetInt();

		return true;
	}

	bool ToMoveMapParameter(const std::string& str_, MoveMapParameter& out_)
	{
		rapidjson::Document doc;
		if (doc.Parse(str_.c_str()).HasParseError())
		{
			std::cerr << "Json::ToMoveMapParam : " << rapidjson::GetParseError_En(doc.GetParseError()) << "\n";

			return false;
		}

		if (!doc.HasMember("MapCode") || !doc["MapCode"].IsInt())
		{
			std::cerr << "Json::ToMoveMapParam : Incorrect Format.\n";
			return false;
		}

		out_.tomapcode = doc["MapCode"].GetInt();

		return true;
	}

	bool ToChatEveryoneParameter(const std::string& str_, ChatEveryoneParameter& out_)
	{
		rapidjson::Document doc;
		if (doc.Parse(str_.c_str()).HasParseError())
		{
			std::cerr << "Json::ToChatEveryoneParam : " << rapidjson::GetParseError_En(doc.GetParseError()) << "\n";

			return false;
		}



		return true;
	}

	bool ToPosInfoParameter(const std::string& str_, PosInfoParameter& out_)
	{
		rapidjson::Document doc;
		if (!doc.Parse(str_.c_str()).HasParseError())
		{
			std::cerr << "Json::ToPosInfoParam : " << rapidjson::GetParseError_En(doc.GetParseError()) << "\n";
			return false;
		}

		if (!doc.HasMember("PosX") || !doc["PosX"].IsFloat() ||
			!doc.HasMember("PosY") || !doc["PosY"].IsFloat() ||
			!doc.HasMember("VelX") || !doc["VelX"].IsFloat() ||
			!doc.HasMember("VelY") || !doc["VelY"].IsFloat())
		{
			std::cerr << "Json::ToPosInfoParam : Incorrect Format.\n";
			return false;
		}

		out_.pos.x = doc["PosX"].GetFloat();
		out_.pos.y = doc["PosY"].GetFloat();
		out_.vel.x = doc["VelX"].GetFloat();
		out_.vel.y = doc["VelY"].GetFloat();

		return true;
	}

private:

	std::wstring_convert<std::codecvt_utf8<wchar_t>> m_cvt;
};
