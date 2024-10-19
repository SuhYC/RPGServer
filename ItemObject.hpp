#pragma once

#include "Vector.hpp"

class ItemObject
{
public:
	void Init(const int itemcode_, const int count_, const int owner_, Vector2& position_)
	{
		m_itemCode = itemcode_;
		m_count = count_;
		m_ownerUsercode = owner_;
		m_position = position_;
	}

	void Clear()
	{
		m_itemCode = 0;
		m_count = 0;
		m_ownerUsercode = 0;
		m_position = Vector2(0.0f,0.0f);
	}

	int GetItemCode() const { return m_itemCode; }
	int GetCount() const { return m_count; }
	int GetOwner() const { return m_ownerUsercode; }
	Vector2 GetPosition() const { return m_position; }

private:
	int m_itemCode;
	int m_count;
	int m_ownerUsercode;
	Vector2 m_position;
};