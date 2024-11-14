#pragma once

#include <unordered_map>

// ���� ���� ������ DB�� ��ȸ�Ͽ� ummap�� �����͸� �־�� ��.
class Shop
{
public:
	void InsertSalesList(const int npccode_, const int itemcode_)
	{
		m_SalesList.emplace(npccode_, itemcode_);

		return;
	}

	// �ش� npc�� ��û�� item�� �Ǹ��ϴ��� Ȯ���Ѵ�.
	bool FindSalesList(const int npccode_, const int itemcode_)
	{
		auto range = m_SalesList.equal_range(npccode_);

		for (auto& itr = range.first; itr != range.second; itr++)
		{
			if (itr->second == itemcode_)
			{
				return true;
			}
		}

		return false;
	}

	void InsertPrice(const int itemcode_, const int price_)
	{
		auto pair = m_PriceTable.emplace(itemcode_, price_);

		// �̹� �ش� �������ڵ�� ���Ե� �����Ͱ� ����.
		if (pair.second == false)
		{
			auto itr = m_PriceTable.find(itemcode_);

			std::cerr << "DB::MakePriceTable : Failed to Emplace on map : (" << itemcode_ << ", " << itr->second << ", " << price_ << ")\n";
		}
		return;
	}

	// �ϴ� Redis�� �������� �ʰ� ������ ���� hashmap���� ������ �ִ´�.
	// �����ܿ��� �ذ��� ������ ���� ������ ���ϸ� �л��Ϸ��� ���� Redis�� �Űܵ� �ɵ�.
	// ������ ������ PC �Ѵ뿡�� ��� �ذ��ؾ� �ϱ⿡ Redis������ �л��س��� ���� ������ �׳� ������ ������ �ִ´�.
	// return -1 : �ش��ϴ� �����ۿ� ���� ���� ����.
	int GetPrice(const int itemcode_)
	{
		auto itr = m_PriceTable.find(itemcode_);

		// ã�� ������ ����
		if (itr == m_PriceTable.end())
		{
			return -1;
		}

		return itr->second;
	}

private:
	std::unordered_multimap<int, int> m_SalesList; // <npccode, itemcode>
	std::unordered_map<int, int> m_PriceTable; // shop���� �ű��?
};