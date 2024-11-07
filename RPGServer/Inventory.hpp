#pragma once

#include <ctime>
#include <algorithm>

const int MAX_SLOT = 30;
const int MAX_COUNT_ON_SLOT = 100;

class Inventory
{
private:
	class Item
	{
	public:
		Item() : itemcode(0), expirationtime(0), count(0)
		{

		}

		Item(const int itemcode_, const time_t extime_, const int count_) : itemcode(itemcode_), expirationtime(extime_), count(count_)
		{

		}

		void Init(const int itemcode_, const time_t extime_, const int count_)
		{
			itemcode = itemcode_;
			expirationtime = extime_;
			count = count_;
			return;
		}

		void Init()
		{
			Init(0, 0, 0);
			return;
		}

		// ���� �����Ѵ�.
		void Init(Item& other_)
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

public:
	// true : ���뼺��, false : �������� Ȥ�� ��������
	bool push_back(const int itemcode_, const time_t extime_, const int count_)
	{
		if (count_ > MAX_COUNT_ON_SLOT)
		{
			return false;
		}

		int remaincnt = count_;

		for (int i = 0; i < MAX_SLOT && remaincnt > 0; i++)
		{
			// ����ִ� ���� ���� (1����)
			if (slot[i].itemcode == 0)
			{
				slot[i].Init(itemcode_, extime_, count_);
				return true;
			}
			// ���� ���Կ� �߰� ������ �з� ����
			else if (slot[i].itemcode == itemcode_ &&
				slot[i].expirationtime == extime_)
			{
				remaincnt -= MAX_COUNT_ON_SLOT - slot[i].count;
			}
		}

		// ���� ���Կ� ���ļ� ���ԺҰ�
		if (remaincnt > 0)
		{
			return false;
		}


		// ���� ���Կ� ���ļ� ���� (2����)
		// ���� ���Ժ��� 100���� ä��� ������ �߰��Ѵ�.
		remaincnt = count_;

		for (int i = 0; i < MAX_SLOT && remaincnt > 0; i++)
		{
			// ��ĥ �� ���� ������
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

private:

	bool cmp(Item& i1, Item& i2)
	{
		if (i1.itemcode == i2.itemcode)
		{
			return i1.expirationtime < i2.expirationtime;
		}
		return i1.itemcode < i2.itemcode;
	}

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