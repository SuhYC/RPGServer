#pragma once

// 간단한 정보는 따로 만들어서 선택화면을 표시해줘야겠다..

class CharInfo
{
public:

	int CharNo;
	char CharName[11]; // 10글자만 허용하기로 했다.

	// 클래스 (혹은 직업)
	// 클래스 코드의 규칙
	// 1의자리 숫자 : 기반이 되는 스탯
	// 1 : Strength
	// 2 : Dexterity
	// 3 : Intelligence
	// 4 : Mentality
	// ex. 1044 : Mentality가 기반.
	int ClassCode;

	int Level;
	int Experience;

	int StatPoint;

	int HealthPoint;
	int ManaPoint;

	int CurrentHealth;
	int CurrentMana;

	int Strength;
	int Dexterity;
	int Intelligence;
	int Mentality;

	int Gold;
	int LastMapCode;
};