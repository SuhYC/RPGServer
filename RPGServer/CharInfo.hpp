#pragma once

class CharInfo
{
public:
	int CharNo;
	int Level;
	int Experience;
	int StatPoint;
	int HealthPoint;
	int ManaPoint;
	int Strength;
	int Dexteriry;
	int Intelligence;
	int Mentality;
	int Gold;
	int LastMapCode;
	wchar_t CharName[11]; // 10글자만 허용하기로 했다.

	std::wstring ToJson()
	{
		return std::wstring(); // rapidjson 반영하고 다시 작성
	}
};