#pragma once

// T(DROPINFO) [MONSTERCODE] [ITEMCODE] [RATIO]
// ratio == num / den;
class MonsterDropInfo
{
public:
	MonsterDropInfo(const int itemcode_, const int num_, const int den_) noexcept : itemcode(itemcode_), num(num_), den(den_)
	{

	}

	MonsterDropInfo(const MonsterDropInfo& other_) noexcept
	{
		itemcode = other_.itemcode;
		num = other_.num;
		den = other_.den;
	}

	MonsterDropInfo(const MonsterDropInfo&& other_) noexcept
	{
		itemcode = other_.itemcode;
		num = other_.num;
		den = other_.den;
	}

	MonsterDropInfo& operator=(const MonsterDropInfo& other_) noexcept
	{
		itemcode = other_.itemcode;
		num = other_.num;
		den = other_.den;
		
		return *this;
	}

	MonsterDropInfo& operator=(const MonsterDropInfo&& other_) noexcept
	{
		itemcode = other_.itemcode;
		num = other_.num;
		den = other_.den;

		return *this;
	}

	int itemcode;
	int num;
	int den;
};