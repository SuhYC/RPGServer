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
	wchar_t CharName[11]; // 10���ڸ� ����ϱ�� �ߴ�.

	std::wstring ToJson()
	{
		return std::wstring(); // rapidjson �ݿ��ϰ� �ٽ� �ۼ�
	}
};