#pragma once

#include <unordered_map>

// 서버 가동 이전에 DB를 조회하여 ummap에 데이터를 넣어둘 것.
class Shop
{
public:
	void InsertSalesList(const int npccode_, const int itemcode_)
	{
		m_SalesList.emplace(npccode_, itemcode_);

		return;
	}

	// 해당 npc가 요청한 item을 판매하는지 확인한다.
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

		// 이미 해당 아이템코드로 삽입된 데이터가 있음.
		if (pair.second == false)
		{
			auto itr = m_PriceTable.find(itemcode_);

			std::cerr << "DB::MakePriceTable : Failed to Emplace on map : (" << itemcode_ << ", " << itr->second << ", " << price_ << ")\n";
		}
		return;
	}

	// 일단 Redis에 저장하지 않고 서버가 직접 hashmap으로 가지고 있는다.
	// 서버단에서 해결할 업무가 많아 서버별 부하를 분산하려면 추후 Redis에 옮겨도 될듯.
	// 지금은 어차피 PC 한대에서 모두 해결해야 하기에 Redis서버를 분산해놓을 수도 없으니 그냥 서버가 가지고 있는다.
	// return -1 : 해당하는 아이템에 대한 정보 없음.
	int GetPrice(const int itemcode_)
	{
		auto itr = m_PriceTable.find(itemcode_);

		// 찾는 아이템 없음
		if (itr == m_PriceTable.end())
		{
			return -1;
		}

		return itr->second;
	}

private:
	std::unordered_multimap<int, int> m_SalesList; // <npccode, itemcode>
	std::unordered_map<int, int> m_PriceTable; // shop으로 옮길까?
};