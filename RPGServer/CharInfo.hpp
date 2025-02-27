#pragma once

// ������ ������ ���� ���� ����ȭ���� ǥ������߰ڴ�..

const int CHARNAMELENGTH = 11;

class CharInfo
{
public:

	int CharNo;
	char CharName[CHARNAMELENGTH]; // 10���ڸ� ����ϱ�� �ߴ�.

	// Ŭ���� (Ȥ�� ����)
	// Ŭ���� �ڵ��� ��Ģ
	// 1���ڸ� ���� : ����� �Ǵ� ����
	// 1 : Strength
	// 2 : Dexterity
	// 3 : Intelligence
	// 4 : Mentality
	// ex. 1044 : Mentality�� ���.
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

	/// <summary>
	/// ���� ����
	/// </summary>
	/// <param name="other_"></param>
	/// <returns></returns>
	CharInfo& operator=(CharInfo& other_)
	{
		if (&other_ != this && other_.CharNo != 0)
		{
			CopyMemory(this, &other_, sizeof(CharInfo));
		}

		return *this;
	}
};