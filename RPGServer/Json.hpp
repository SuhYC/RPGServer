#pragma once

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <string>
#include "CharInfo.hpp"
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
			wcscpy(out_.CharName, wstr.c_str());

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

	}

private:

	std::wstring_convert<std::codecvt_utf8<wchar_t>> m_cvt;
};