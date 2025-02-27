#pragma once

#include <ctime>

class Item
{
public:
	Item() : itemcode(0), expirationtime(0), count(0)
	{

	}

	Item(const int itemcode_, const time_t extime_, const int count_) : itemcode(itemcode_), expirationtime(extime_), count(count_)
	{

	}

	bool operator<(Item& other_)
	{
		if (itemcode == other_.itemcode)
		{
			return expirationtime < other_.expirationtime;
		}
		return itemcode < other_.itemcode;
	}

	/// <summary>
	/// 특정한 아이템으로 초기화
	/// </summary>
	/// <param name="itemcode_"></param>
	/// <param name="extime_"></param>
	/// <param name="count_"></param>
	void Init(const int itemcode_, const time_t extime_, const int count_)
	{
		itemcode = itemcode_;
		expirationtime = extime_;
		count = count_;
		return;
	}

	/// <summary>
	/// 빈슬롯으로 초기화
	/// </summary>
	void Init()
	{
		Init(0, 0, 0);
		return;
	}

	// 값을 복사한다.
	void Init(Item& other_)
	{
		Init(other_.itemcode, other_.expirationtime, other_.count);
		return;
	}

	void Init(const Item& other_)
	{
		Init(other_.itemcode, other_.expirationtime, other_.count);
		return;
	}

	void Swap(Item& other_)
	{
		Item tmp(other_);
		other_.Init(*this);
		this->Init(tmp);
		return;
	}

	int itemcode;
	time_t expirationtime;
	int count;
};