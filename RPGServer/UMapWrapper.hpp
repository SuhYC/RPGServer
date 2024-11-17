#pragma once

#include <utility>
#include <unordered_map>


template <typename T1, typename T2>
class UMapWrapper
{
public:
	// 이미 key에 해당하는 데이터가 삽입되어있다면 실패한다.
	void Insert(const T1& key_, const T2& value_)
	{
		umap.emplace(key_, value_);
		return;
	}
	// 이미 key에 해당하는 데이터가 삽입되어있다면 실패한다.
	void Insert(const T1&& key_, const T2& value_)
	{
		umap.emplace(key_, value_);
		return;
	}
	// 이미 key에 해당하는 데이터가 삽입되어있다면 실패한다.
	void Insert(const T1& key_, const T2&& value_)
	{
		umap.emplace(key_, value_);
		return;
	}
	// 이미 key에 해당하는 데이터가 삽입되어있다면 실패한다.
	void Insert(const T1&& key_, const T2&& value_)
	{
		umap.emplace(key_, value_);
		return;
	}

	// 데이터가 없다면 기본생성자를 리턴한다.
	T2 Get(const T1& itemcode_)
	{
		auto itr = umap.find(itemcode_);

		// 찾는 아이템 없음
		if (itr == umap.end())
		{
			return T2();
		}

		return itr->second;
	}

	// 데이터가 없다면 기본생성자를 리턴한다.
	T2 Get(const T1&& itemcode_)
	{
		auto itr = umap.find(itemcode_);

		// 찾는 아이템 없음
		if (itr == umap.end())
		{
			return T2();
		}

		return itr->second;
	}

private:
	std::unordered_map<T1, T2> umap;
};

template <typename T1, typename T2>
class UMultiMapWrapper
{
public:
	// 이미 key에 해당하는 데이터가 삽입되어있다면 실패한다.
	void Insert(const T1& key_, const T2& value_)
	{
		ummap.emplace(key_, value_);

		return;
	}

	// key-value가 일치하는 데이터가 있는지 여부를 booline으로 리턴한다.
	bool Find(const T1& key_, const T2& value_)
	{
		auto range = ummap.equal_range(key_);

		for (auto& itr = range.first; itr != range.second; itr++)
		{
			if (itr->second == value_)
			{
				return true;
			}
		}

		return false;
	}

	// 해당 key에 맞는 버킷의 데이터를 반복자로 리턴한다. (equal_range)
	// ret.first : begin()
	// ret.second : end()
	// 템플릿 타입으로 인해 발생하는 지연된 이름 해석 문제(dependent name resolution) -> typename 추가
	// https://learn.microsoft.com/ko-kr/cpp/cpp/name-resolution-for-dependent-types?view=msvc-170
	std::pair<typename std::unordered_multimap<T1, T2>::iterator, typename std::unordered_multimap<T1, T2>::iterator> Get(T1& key_)
	{
		return ummap.equal_range(key_);
	}

private:
	std::unordered_multimap<T1, T2> ummap;
};