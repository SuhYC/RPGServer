#pragma once

#include <algorithm>
#include "Item.hpp"

const int MAX_SLOT = 64;
const int MAX_COUNT_ON_SLOT = 100;

class Inventory
{
public:
	Inventory()
	{
		for (int i = 0; i < MAX_SLOT; i++)
		{
			slot[i].Init();
		}
	}

	// true : ���뼺��, false : �������� Ȥ�� ��������
	// ���� ���Կ� �켱���� ������ �� �ֵ��� �Ѵ�
	// ������ �κ��丮 ������ redis�� �ֱ� ������ �����ϴ��� �ݿ����� ������ ������.
	// �ϴ� �����ϸ鼭 ä������ �Ǹ� true��, �ȵǸ� false�� ��ȯ�ϸ�
	// false�� ��ȯ�� ��� redis�� �ݿ��ϸ� �ȵȴ�.
	bool push_back(const int itemcode_, const time_t extime_, const int count_)
	{
		if (count_ > MAX_COUNT_ON_SLOT)
		{
			return false;
		}

		int remaincnt = count_;

		// ������ �ش� �������� �ִ��� Ȯ�� �� �ش� ���Կ� ä�� �� �ִ¸�ŭ ä���.
		for (int i = 0; i < MAX_SLOT && remaincnt > 0; i++)
		{
			if (slot[i].itemcode == itemcode_ &&
				slot[i].expirationtime == extime_)
			{
				int available = MAX_COUNT_ON_SLOT - slot[i].count;

				if (available >= remaincnt)
				{
					slot[i].count += remaincnt;
					remaincnt = 0;
				}
				else
				{
					remaincnt -= available;
					slot[i].count = MAX_COUNT_ON_SLOT;
				}
			}
		}

		// ��
		if (remaincnt < 1)
		{
			return true;
		}

		// ����ִ� ���Կ� �������� ��´�.
		for (int i = 0; i < MAX_SLOT && remaincnt > 0; i++)
		{
			if (slot[i].itemcode == 0)
			{
				slot[i].Init(itemcode_, extime_, remaincnt);
				return true;
			}
		}

		// �ᱹ ���� ����
		return false;
	}

	// ������ �������� �������� �ʵ��� �ٲپ��
	void sort()
	{
		std::sort(slot, slot + MAX_SLOT);

		// ���� ��ġ��
		for (int idx = 0; idx < MAX_SLOT; idx++)
		{
			// �󽽷��̳� ���� ������ �ǳʶ�
			if (slot[idx].itemcode == 0 ||
				slot[idx].count == MAX_COUNT_ON_SLOT)
			{
				continue;
			}

			// ���� ���԰� ������ �������̰� ������ ������ ��� �� ����
			for (int after = idx + 1; slot[idx].count < MAX_COUNT_ON_SLOT && after < MAX_SLOT; after++)
			{
				if (slot[idx].itemcode != slot[after].itemcode ||
					slot[idx].expirationtime != slot[after].expirationtime)
				{
					continue;
				}

				// �ش� ���Կ� ���� �Ұ� (�Ϻθ� ���)
				if (slot[idx].count + slot[after].count > MAX_COUNT_ON_SLOT)
				{
					slot[after].count -= MAX_COUNT_ON_SLOT - slot[idx].count;
					slot[idx].count = MAX_COUNT_ON_SLOT;
				}
				// �ش� ���Կ� ���� ���� (�� ������ �󽽷��� ��)
				else
				{
					slot[idx].count += slot[after].count;
					slot[after].Init();
				}
			}
		}

		// �� �������� ������ ����
		int emptySlotIndex = GetEmptySlot();
		int usingSlotIndex = GetUsingSlot();

		// �˻��� �ε����� ��ȿ�� ��� �ݺ�
		while (emptySlotIndex != -1 &&
			usingSlotIndex != -1)
		{
			// �տ� �󽽷��� �ִ� ��� ���
			if (emptySlotIndex < usingSlotIndex)
			{
				slot[emptySlotIndex].Swap(slot[usingSlotIndex]);
			}

			// �ش� ���� ���Ŀ��� ��˻� (��� ������ ���ٸ� �˻��ϴٰ� �ε��� ������ �����.)
			emptySlotIndex = GetEmptySlot(emptySlotIndex + 1);
			usingSlotIndex = GetUsingSlot(usingSlotIndex + 1);
		}

		return;
	}

	// �κ��丮 ���ʿ� �ִ� ������ �켱���� ����Ѵ�.
	// ���ȿ���� �ٸ� ������ �����ϵ��� ����.
	// true : ���뼺��, false : �����ۺ������� ���� ����
	bool Use(const int itemcode_, const int count_ = 1)
	{
		// �����ϴ°���..
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

		// ������ ����
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

	bool Swap(const int idx1, const int idx2)
	{
		int code = slot[idx1].itemcode;
		int count = slot[idx1].count;
		time_t extime = slot[idx1].expirationtime;

		slot[idx1].itemcode = slot[idx2].itemcode;
		slot[idx1].count = slot[idx2].count;
		slot[idx1].expirationtime = slot[idx2].expirationtime;

		slot[idx2].itemcode = code;
		slot[idx2].count = count;
		slot[idx2].expirationtime = extime;

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
	// ����ִ� ������ �ε����� ����.
	// ����ִ� ������ ���ٸ� -1�� ����.
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