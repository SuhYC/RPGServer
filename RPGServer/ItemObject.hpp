#pragma once

#include "Vector.hpp"
#include "Json.hpp"

// 필드에 뿌려진 아이템에 대한 정보이다.
// 1번 아이템을 Gold로 표기해야겠다. (0번 아이템은 빈칸)
class ItemObject
{
public:
	void Init(const unsigned int idx_, const int itemcode_, const time_t extime_, const int count_, const int owner_, Vector2& position_, time_t ownershipPeriod_ = 0)
	{
		m_idx = idx_;

		CreatedTime = std::chrono::steady_clock::now();

		m_itemCode = itemcode_;
		m_exTime = extime_;
		m_count = count_;
		m_ownerCharcode = owner_;
		m_position = position_;
		m_OwnershipPeriod = ownershipPeriod_;
	}

	void Clear()
	{
		m_idx = 0;
		m_itemCode = 0;
		m_exTime = 0;
		m_count = 0;
		m_ownerCharcode = 0;
		m_position = Vector2(0.0f,0.0f);
	}

	int GetIdx() const { return m_idx; }
	int GetItemCode() const { return m_itemCode; }
	time_t GetExTime() const { return m_exTime; }
	int GetCount() const { return m_count; }
	int GetOwner() const { return m_ownerCharcode; }
	Vector2 GetPosition() { return m_position; }

	// 아이템 습득과정에서 문제가 생겼을 경우
	// 다시 맵에 아이템을 반환해야하는지 확인하는 함수이다.
	bool HaveToReturn() const
	{
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		std::chrono::duration<double> elapsed_seconds = now - CreatedTime;

		return elapsed_seconds.count() > 59.0; // 여유롭게 1초 미리 없애도 될듯.. 일단 Discard가 60초니까 
	}

	void GetInfo(CreateObjectResponse& out_) const
	{
		out_.itemcode = m_itemCode;
		out_.extime = m_exTime;
		out_.count = m_count;
		out_.position = m_position;
	}

	bool CanGet(const int charcode_) const
	{
		// 이 경우는 아이템소멸을 위한 경우.
		if (charcode_ == 0)
		{
			return true;
		}

		// 이 경우는 소유권이 없는 아이템.
		if (m_ownerCharcode == 0)
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
	unsigned int m_idx;
	int m_itemCode;
	time_t m_exTime;
	int m_count;
	int m_ownerCharcode;
	Vector2 m_position;
	time_t m_OwnershipPeriod; // 해당 시간까지는 타인이 줍지 못한다.
	std::chrono::steady_clock::time_point CreatedTime;
};