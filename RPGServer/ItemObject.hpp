#pragma once

#include "Vector.hpp"

// 필드에 뿌려진 아이템에 대한 정보이다.
// 1번 아이템을 Gold로 표기해야겠다. (0번 아이템은 빈칸)
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
		// 이 경우는 아이템소멸을 위한 경우.
		if (charcode_ == 0)
		{
			return true;
		}

		// 소유권을 가진 유저의 요청
		if (charcode_ == m_ownerCharcode)
		{
			return true;
		}

		time_t now = time(NULL);

		// 소유권이 사라진 이후의 요청
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
	time_t m_OwnershipPeriod; // 해당 시간까지는 타인이 줍지 못한다.
};