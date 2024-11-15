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
* wstring�� codecvt�� �Ͱ� string�� ���� �ٸ� ���� ������.
* -> ���ڵ��� ���� �ٸ��� ����.
* ���ڿ��� string���θ� �Է¹޴°� ������.
* 
* ����ü->json : struct -> string
* json->����ü : string -> struct
* 
* ĳ���� �г��� ����
* 1. ĳ���� ���� ���� �� �г��� �ߺ�Ȯ�� �� ������� �����ϴ� ������ ��ģ��.
* 2. ������� �����ϴ� ���� �߿� ����� ���·� �ٸ� ������ ������ �� ������ �Ѵ�.
*  ���� : Redis�� ���� �Ŵµ� �����ߴٸ� User ������ ȹ���� ���� ����� �� �ֵ��� �Ѵ�.
*  ���� ���� 1���� �ð��� �ο��Ͽ� �����Ѵ�. 1�� ���� �������� ������ ���� �����Ѵ�. ���� ��ȿ�Ⱓ�� 2������ �����Ѵ�.
*  - ����Ѵ� ���� (ĳ���� ���� ��û)
*	������ �ش� �������� �ɷ��ִ��� User�� ���� Ȯ���ϰ� ���� �ִٸ� ĳ���͸� ����
*  - ������� �ʴ´� ����
*   �ش� ������ ���� �ִٸ� ������ �����Ѵ�.
*/ 


enum class MessageType
{
	SIGNIN,
	SIGNUP,
	MODIFY_PW,
	GET_CHAR_LIST,
	GET_CHAR_INFO,
	RESERVE_CHAR_NAME, // ĳ���� ���� �� �г����� �Է��� ��� �̸� ������ �� ��, ������ �г����� ��� ����� �� ���θ� �����Ѵ�.
	CANCEL_CHAR_NAME_RESERVE, // RESERVE_CHAR_NAME���� ������ �г����� ����Ѵ�.
	CREATE_CHAR, // ĳ���� ���� ����. DB�� ĳ���� ������ ����Ѵ�.
	SELECT_CHAR,
	PERFORM_SKILL,
	GET_OBJECT,
	BUY_ITEM,
	DROP_ITEM,
	USE_ITEM,
	MOVE_MAP, // �� �̵� ��û
	CHAT_EVERYONE, // �ش� ���� ��� �ο����� ä�� (�ٸ� ������ ä���� �� �����غ���.)

	POS_INFO, // ĳ������ ��ġ, �ӵ� � ���� ������ ������Ʈ�ϴ� �Ķ����
	LAST = POS_INFO // enum class�� �����Ǹ� ������ ���ҷ� ������ ��
};

enum class RESULTCODE
{
	SUCCESS,
	WRONG_PARAM,					// ��û��ȣ�� ���� �ʴ� �Ķ����
	SYSTEM_FAIL,					// �ý����� ����
	SIGNIN_FAIL,					// ID�� PW�� ���� ����
	SIGNIN_ALREADY_HAVE_SESSION,	// �̹� �α��ε� ����
	SIGNUP_FAIL,					// ID��Ģ�̳� PW��Ģ�� ���� ����
	SIGNUP_ALREADY_IN_USE,			// �̹� ������� ID
	WRONG_ORDER,					// ��û�� ��Ȳ�� ���� ����
	MODIFIED_MAPCODE,				// �ش� ��û�� ���� ���� ���ڵ�� ���� ������ ������ �ִ� �ش� ������ ���ڵ尡 ������.
	REQ_FAIL,						// ���� ���ǿ� ���� �ʾ� ������ ������ (�̹� ����� ������ ��..)
	SEND_INFO,						// �ش� ������ ��û�� ���� ���޵� ������ �ƴ� �������� (ä��, �ʺ�ȭ, �ٸ��÷��̾� ��ȣ�ۿ�...)
	UNDEFINED						// �˼����� ����
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
	int questno; // ��й�ȣ ���� ����
	std::string ans; // ��й�ȣ ���� ��
	std::string hint; // ��й�ȣ ���� ���� ��Ʈ(��������)
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
	// ĳ���͸� ����� ��� ������ �����ϰ� ����
	// ������ �����ϰ� ĳ���͸� ����� �ش� ���� ������ �����ϰ� ���� ��� �ʿ�
};

struct SelectCharParameter
{
	int charcode;
};

struct PerformSkillParameter
{
	int mapcode;
	int monsterbitmask; // 1011 : 0��, 1��, 3�� ���Ϳ��� Ÿ��, 32bit�ϱ� �� ���� ���ʹ� 32�������� ��
	int skillcode;
	long long timeValue; // Ŭ���̾�Ʈ���� ������ �ð�, �ش� �ð��� ������� ��������Ͽ� ������ ���
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

	// out_ �Ű������� ���� ��ü�� �ʱ�ȭ�ȴ�. ����.
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
		out_.Init(array.Size()); // �ʱ�ȭ

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
			// rapidjson::Document::AddMember���� std::string�� ȣȯ���� �ʴ´�.
			// rapidjson::Value�� ��ȯ�Ͽ� �����Ѵ�.
			
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

	// out_ �Ű������� ���� ��ü�� �ʱ�ȭ�ȴ�. ����.
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

	// CancelReserve�� ������ parameter�� ����Ѵ�.
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
