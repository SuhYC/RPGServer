#pragma once

#include <utility>
#include <unordered_map>


template <typename T1, typename T2>
class UMapWrapper
{
public:
	// �̹� key�� �ش��ϴ� �����Ͱ� ���ԵǾ��ִٸ� �����Ѵ�.
	void Insert(const T1& key_, const T2& value_)
	{
		umap.emplace(key_, value_);
		return;
	}
	// �̹� key�� �ش��ϴ� �����Ͱ� ���ԵǾ��ִٸ� �����Ѵ�.
	void Insert(const T1&& key_, const T2& value_)
	{
		umap.emplace(key_, value_);
		return;
	}
	// �̹� key�� �ش��ϴ� �����Ͱ� ���ԵǾ��ִٸ� �����Ѵ�.
	void Insert(const T1& key_, const T2&& value_)
	{
		umap.emplace(key_, value_);
		return;
	}
	// �̹� key�� �ش��ϴ� �����Ͱ� ���ԵǾ��ִٸ� �����Ѵ�.
	void Insert(const T1&& key_, const T2&& value_)
	{
		umap.emplace(key_, value_);
		return;
	}

	// �����Ͱ� ���ٸ� �⺻�����ڸ� �����Ѵ�.
	T2 Get(const T1& itemcode_)
	{
		auto itr = umap.find(itemcode_);

		// ã�� ������ ����
		if (itr == umap.end())
		{
			return T2();
		}

		return itr->second;
	}

	// �����Ͱ� ���ٸ� �⺻�����ڸ� �����Ѵ�.
	T2 Get(const T1&& itemcode_)
	{
		auto itr = umap.find(itemcode_);

		// ã�� ������ ����
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
	// �̹� key�� �ش��ϴ� �����Ͱ� ���ԵǾ��ִٸ� �����Ѵ�.
	void Insert(const T1& key_, const T2& value_)
	{
		ummap.emplace(key_, value_);

		return;
	}

	// key-value�� ��ġ�ϴ� �����Ͱ� �ִ��� ���θ� booline���� �����Ѵ�.
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

	// �ش� key�� �´� ��Ŷ�� �����͸� �ݺ��ڷ� �����Ѵ�. (equal_range)
	// ret.first : begin()
	// ret.second : end()
	// ���ø� Ÿ������ ���� �߻��ϴ� ������ �̸� �ؼ� ����(dependent name resolution) -> typename �߰�
	// https://learn.microsoft.com/ko-kr/cpp/cpp/name-resolution-for-dependent-types?view=msvc-170
	std::pair<typename std::unordered_multimap<T1, T2>::iterator, typename std::unordered_multimap<T1, T2>::iterator> Get(T1& key_)
	{
		return ummap.equal_range(key_);
	}

private:
	std::unordered_multimap<T1, T2> ummap;
};