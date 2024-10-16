#pragma once

// 간단한 정보는 따로 만들어서 선택화면을 표시해줘야겠다..

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
};