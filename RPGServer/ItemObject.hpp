#pragma once

#include "Vector.hpp"

// �ʵ忡 �ѷ��� �����ۿ� ���� �����̴�.
// 1�� �������� Gold�� ǥ���ؾ߰ڴ�. (0�� �������� ��ĭ)
class ItemObject
{
public:
	void Init(const int itemcode_, const int count_, const int owner_, Vector2& position_, time_t ownershipPeriod_ = 0)
	{
		m_itemCode = itemcode_;
		m_count = count_;
		m_ownerCharcode = owner_;
		m_position = position_;
		m_OwnershipPeriod = ownershipPeriod_;
	}

	void Clear()
	{
		m_itemCode = 0;
		m_count = 0;
		m_ownerCharcode = 0;
		m_position = Vector2(0.0f,0.0f);
	}

	int GetItemCode() const { return m_itemCode; }
	int GetCount() const { return m_count; }
	int GetOwner() const { return m_ownerCharcode; }
	Vector2 GetPosition() const { return m_position; }

	bool CanGet(const int charcode_) const
	{
		// �� ���� �����ۼҸ��� ���� ���.
		if (charcode_ == 0)
		{
			return true;
		}

		// �������� ���� ������ ��û
		if (charcode_ == m_ownerCharcode)
		{
			return true;
		}

		time_t now = time(NULL);

		// �������� ����� ������ ��û
		if (now >= m_OwnershipPeriod)
		{
			return true;
		}

		return false;
	}

private:
	int m_itemCode;
	int m_count;
	int m_ownerCharcode;
	Vector2 m_position;
	time_t m_OwnershipPeriod; // �ش� �ð������� Ÿ���� ���� ���Ѵ�.
};