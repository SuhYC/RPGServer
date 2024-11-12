#pragma once

#include <algorithm>
#include "Item.hpp"

const int MAX_SLOT = 30;
const int MAX_COUNT_ON_SLOT = 100;

class Inventory
{
public:
	// true : 적용성공, false : 수량문제 혹은 공간부족
	bool push_back(const int itemcode_, const time_t extime_, const int count_)
	{
		if (count_ > MAX_COUNT_ON_SLOT)
		{
			return false;
		}

		int remaincnt = count_;

		for (int i = 0; i < MAX_SLOT && remaincnt > 0; i++)
		{
			// 비어있는 슬롯 있음 (1순위)
			if (slot[i].itemcode == 0)
			{
				slot[i].Init(itemcode_, extime_, count_);
				return true;
			}
			// 기존 슬롯에 추가 가능한 분량 세기
			else if (slot[i].itemcode == itemcode_ &&
				slot[i].expirationtime == extime_)
			{
				remaincnt -= MAX_COUNT_ON_SLOT - slot[i].count;
			}
		}

		// 기존 슬롯에 합쳐서 삽입불가
		if (remaincnt > 0)
		{
			return false;
		}


		// 기존 슬롯에 합쳐서 삽입 (2순위)
		// 앞쪽 슬롯부터 100개를 채우기 전까지 추가한다.
		remaincnt = count_;

		for (int i = 0; i < MAX_SLOT && remaincnt > 0; i++)
		{
			// 합칠 수 없는 아이템
			if (slot[i].itemcode != itemcode_ ||
				slot[i].expirationtime != extime_)
			{
				continue;
			}

			if (slot[i].count + remaincnt > MAX_COUNT_ON_SLOT)
			{
				remaincnt -= MAX_COUNT_ON_SLOT - slot[i].count;
				slot[i].Init(itemcode_, extime_, MAX_COUNT_ON_SLOT);
			}
			else
			{
				slot[i].Init(itemcode_, extime_, slot[i].count + remaincnt);
				remaincnt = 0;
			}
		}

		return true;
	}

	void sort()
	{
		std::sort(slot, slot + MAX_SLOT);

		// 수량 합치기
		for (int idx = 0; idx < MAX_SLOT; idx++)
		{
			// 빈슬롯이나 꽉찬 슬롯은 건너뜀
			if (slot[idx].itemcode == 0 ||
				slot[idx].count == MAX_COUNT_ON_SLOT)
			{
				continue;
			}

			// 다음 슬롯과 동일한 아이템이고 수량을 앞으로 당길 수 있음
			for (int after = idx + 1; slot[idx].count < MAX_COUNT_ON_SLOT && after < MAX_SLOT; after++)
			{
				if (slot[idx].itemcode != slot[after].itemcode ||
					slot[idx].expirationtime != slot[after].expirationtime)
				{
					continue;
				}

				// 해당 슬롯에 병합 불가 (일부만 당김)
				if (slot[idx].count + slot[after].count > MAX_COUNT_ON_SLOT)
				{
					slot[after].count -= MAX_COUNT_ON_SLOT - slot[idx].count;
					slot[idx].count = MAX_COUNT_ON_SLOT;
				}
				// 해당 슬롯에 병합 가능 (뒷 슬롯은 빈슬롯이 됨)
				else
				{
					slot[idx].count += slot[after].count;
					slot[after].Init();
				}
			}
		}

		// 빈 슬롯으로 아이템 당기기
		int emptySlotIndex = GetEmptySlot();
		int usingSlotIndex = GetUsingSlot();

		// 검색된 인덱스가 유효한 경우 반복
		while (emptySlotIndex != -1 &&
			usingSlotIndex != -1)
		{
			// 앞에 빈슬롯이 있는 경우 당김
			if (emptySlotIndex < usingSlotIndex)
			{
				slot[emptySlotIndex].Swap(slot[usingSlotIndex]);
			}

			// 해당 슬롯 이후에서 재검색 (당길 슬롯이 없다면 검색하다가 인덱스 범위를 벗어날것.)
			emptySlotIndex = GetEmptySlot(emptySlotIndex + 1);
			usingSlotIndex = GetUsingSlot(usingSlotIndex + 1);
		}

		return;
	}

	// 인벤토리 앞쪽에 있는 아이템 우선으로 사용한다.
	// 사용효과는 다른 곳에서 적용하도록 하자.
	// true : 적용성공, false : 아이템부족으로 인한 실패
	bool Use(const int itemcode_, const int count_ = 1)
	{
		// 사용안하는거잖..
		if (count_ < 1)
		{
			return true;
		}

		int remaincnt = count_;

		for (int i = 0; remaincnt > 0 && i < MAX_SLOT; i++)
		{
			if (slot[i].itemcode != itemcode_)
			{
				continue;
			}

			remaincnt -= slot[i].count;
		}

		// 아이템 부족
		if (remaincnt > 0)
		{
			return false;
		}

		remaincnt = count_;

		for (int i = 0; remaincnt > 0 && i < MAX_SLOT; i++)
		{
			if (slot[i].itemcode != itemcode_)
			{
				continue;
			}

			if (remaincnt >= slot[i].count)
			{
				remaincnt -= slot[i].count;
				slot[i].Init();
			}
			else
			{
				slot[i].count -= remaincnt;
				remaincnt = 0;
			}
		}

		return true;
	}

	bool Drop(const int slotIdx_, const int count_ = 1)
	{
		if (slotIdx_ >= MAX_SLOT || slotIdx_ < 0 || count_ <= 0)
		{
			return false;
		}

		if (slot[slotIdx_].count < count_)
		{
			return false;
		}
		else if (slot[slotIdx_].count == count_)
		{
			slot[slotIdx_].Init();
		}
		else
		{
			slot[slotIdx_].count -= count_;
		}

		return true;
	}

	Item& operator[](const int slotIdx_)
	{
		//if (slotIdx_ < 0 || slotIdx_ >= MAX_SLOT)
		//{
		//	throw std::out_of_range("");
		//}
		return slot[slotIdx_];
	}

private:
	// 비어있는 슬롯의 인덱스를 리턴.
	// 비어있는 슬롯이 없다면 -1을 리턴.
	int GetEmptySlot(const int start = 0) const
	{
		for (int i = start; i < MAX_SLOT; i++)
		{
			if (slot[i].itemcode == 0)
			{
				return i;
			}
		}

		return -1;
	}

	int GetUsingSlot(const int start = 0) const
	{
		for (int i = start; i < MAX_SLOT; i++)
		{
			if (slot[i].itemcode != 0)
			{
				return i;
			}
		}

		return -1;
	}

	Item slot[MAX_SLOT];
};