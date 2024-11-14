#pragma once

#include <unordered_map>

class MapEdge
{
public:
	// ���� ���� �� �ش� �Լ��� MapEdge�� ���� ������ ����Ͽ����Ѵ�.
	void Insert(const int fromMapCode_, const int toMapCode_) noexcept
	{
		m_mapEdge.emplace(fromMapCode_, toMapCode_);

		return;
	}

	// from�ʿ��� to������ �� �� �ִ��� Ȯ���ϴ� �Լ�
	// �� �ʿ��� �� �� �ִ� ��ΰ� ���� ���� ���̱� ������ multimap�� ���� ��ȸ�Ѵ�.
	// fromMapcode_�� ��ȸ�ϴ� �������� O(1), ��ȸ������� toMapCode_�� Ȯ���ϴµ� O(n)
	bool Find(const int fromMapCode_, const int toMapCode_)
	{
		auto range = m_mapEdge.equal_range(fromMapCode_);

		for (auto& itr = range.first; itr != range.second; itr++)
		{
			if (itr->second == toMapCode_)
			{
				return true;
			}
		}

		return false;
	}

private:
	std::unordered_multimap<int, int> m_mapEdge; // param1�ʿ��� param2������ �̵��������� ��Ÿ��. -> DB�۾� �ʿ�
};