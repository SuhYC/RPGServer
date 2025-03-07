#pragma once

#include "Vector.hpp"
#include "Json.hpp"

// �ʵ忡 �ѷ��� �����ۿ� ���� �����̴�.
// 1�� �������� Gold�� ǥ���ؾ߰ڴ�. (0�� �������� ��ĭ)
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

	// ������ ����������� ������ ������ ���
	// �ٽ� �ʿ� �������� ��ȯ�ؾ��ϴ��� Ȯ���ϴ� �Լ��̴�.
	bool HaveToReturn() const
	{
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		std::chrono::duration<double> elapsed_seconds = now - CreatedTime;

		return elapsed_seconds.count() > 59.0; // �����Ӱ� 1�� �̸� ���ֵ� �ɵ�.. �ϴ� Discard�� 60�ʴϱ� 
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
		// �� ���� �����ۼҸ��� ���� ���.
		if (charcode_ == 0)
		{
			return true;
		}

		// �� ���� �������� ���� ������.
		if (m_ownerCharcode == 0)
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
	unsigned int m_idx;
	int m_itemCode;
	time_t m_exTime;
	int m_count;
	int m_ownerCharcode;
	Vector2 m_position;
	time_t m_OwnershipPeriod; // �ش� �ð������� Ÿ���� ���� ���Ѵ�.
	std::chrono::steady_clock::time_point CreatedTime;
};