#pragma once

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"
#include <string>
#include "CharInfo.hpp"
#include "CharList.hpp"
#include <codecvt>

/*
* wstring을 codecvt한 것과 string은 서로 다른 값을 가진다.
* -> 인코딩이 서로 다르기 때문.
*  문자열은 wstring으로만 입력받는게 나을듯.
* 구조체->json : struct -> string
* json->구조체 : string -> struct
* 
* !!json문자열은 레디스영역을 벗어나지 않도록 할 것!!
*/ 


enum class MessageType
{
	SIGNIN,
	SIGNUP,
	MODIFY_PW,
	GET_CHAR_LIST,
	GET_CHAR_INFO,
	SELECT_CHAR,
	PERFORM_SKILL,
	GET_OBJECT,
	BUY_ITEM,
	DROP_ITEM
};

struct ReqMessage
{
	MessageType type;
	std::string msg; // parameter of the type to json
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
	unsigned char questno; // 비밀번호 변경 문제
	std::string ans; // 비밀번호 변경 답
	std::string hint; // 비밀번호 변경 문제 힌트(유저기입)
};

struct ModifyPWParameter
{
	std::string id;
	std::string ans;
};

struct GetCharListParameter
{
	int usercode;
};

struct GetCharInfoParameter
{
	int charcode;
};

struct SelectCharParameter
{
	int charcode;
};

struct PerformSkillParameter
{

};

struct BuyItemParameter
{

};

struct GetItemParameter
{

};

class JsonMaker
{
public:
	bool ToJsonString(const CharInfo* pInfo_, std::string& out_)
	{
		if (pInfo_ == nullptr)
		{
			return false;
		}

		rapidjson::Document doc;
		doc.SetObject();

		rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

		// structType
		doc.AddMember("Type", "CharInfo", allocator);

		// int Members
		doc.AddMember("CharNo", pInfo_->CharNo, allocator);
		doc.AddMember("Level", pInfo_->Level, allocator);
		doc.AddMember("Experience", pInfo_->Experience, allocator);
		doc.AddMember("StatPoint", pInfo_->StatPoint, allocator);
		doc.AddMember("HealthPoint", pInfo_->HealthPoint, allocator);
		doc.AddMember("ManaPoint", pInfo_->ManaPoint, allocator);
		doc.AddMember("Strength", pInfo_->Strength, allocator);
		doc.AddMember("Dexterity", pInfo_->Dexterity, allocator);
		doc.AddMember("Intelligence", pInfo_->Intelligence, allocator);
		doc.AddMember("Mentality", pInfo_->Mentality, allocator);
		doc.AddMember("Gold", pInfo_->Gold, allocator);
		doc.AddMember("LastMapCode", pInfo_->LastMapCode, allocator);

		// wchar_t[] Member
		std::string utf8str = m_cvt.to_bytes(pInfo_->CharName);
		doc.AddMember("CharName", rapidjson::Value().SetString(utf8str.c_str(), static_cast<rapidjson::SizeType>(utf8str.length())), allocator);

		// Make JsonString
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		doc.Accept(writer);

		out_ = buffer.GetString();

		return true;
	}

	bool ToCharInfo(std::string& jsonStr_, CharInfo& out_)
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
	bool ToCharList(std::string& str_, CharList& out_)
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

	bool ToReqMessage(std::string& str_, ReqMessage& out_)
	{
		rapidjson::Document doc;
		if (!doc.Parse(str_.c_str()).HasParseError())
		{
			std::cerr << "Json::ToReqMessage : " << rapidjson::GetParseError_En(doc.GetParseError()) << "\n";
			return false;
		}

		if (!doc.HasMember("Type") || !doc["Type"].IsInt() ||
			!doc.HasMember("msg") || !doc["msg"].IsString())
		{
			std::cerr << "Json::ToReqMessage : Incorrect Format.\n";
			return false;
		}

		int type = doc["Type"].GetInt();

		if (type > static_cast<int>(MessageType::DROP_ITEM) || type < static_cast<int>(MessageType::SIGNIN))
		{
			std::cerr << "Json::ToReqMessage : Not Defined MessageType.\n";
			return false;
		}

		out_.type = static_cast<MessageType>(type);
		out_.msg = std::string(doc["msg"].GetString());

		return true;
	}

	bool ToSignInParameter(std::string& str_, SignInParameter& out_)
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

	bool ToSignUpParameter(std::string& str_, SignUpParameter& out_)
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

		int nData = doc["QuestNo"].GetInt();

		if (nData > 255)
		{
			std::cerr << "Json::ToSignUpParam : To Large QuestNo.\n";
			return false;
		}

		out_.questno = static_cast<unsigned char>(nData);
		out_.ans = doc["Answer"].GetString();
		out_.hint = doc["Hint"].GetString();

		return true;
	}

	bool ToModifyPWParameter(std::string& str_, ModifyPWParameter& out_)
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

	bool ToGetCharListParameter(std::string& str_, GetCharListParameter& out_)
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

	bool ToGetCharInfoParameter(std::string& str_, GetCharInfoParameter& out_)
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

	bool ToSelectCharParameter(std::string& str_, SelectCharParameter& out_)
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

private:

	std::wstring_convert<std::codecvt_utf8<wchar_t>> m_cvt;
};
