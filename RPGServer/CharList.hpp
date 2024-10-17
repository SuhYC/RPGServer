#pragma once
#include <stdexcept>

// DB���� ���� �����͸� ������ Ŭ����.
// ���� Jsonȭ �Ͽ� ���� ����
// ĳ������ �ڵ常 �޾ƿ´�. intŸ��.
// ������ �����ұ�? �����ϸ� ������ �����Ҵ��� ������ ����
class CharList
{
public:
	~CharList()
	{
		if (mp_Data != nullptr)
		{
			delete[] mp_Data;
		}
	}

	void Init(const size_t count_)
	{
		if (mp_Data != nullptr)
		{
			delete[] mp_Data;
		}

		try {
			mp_Data = new int[count_];
			m_Size = count_;
		}
		catch (std::bad_alloc e)
		{
			std::cerr << "�޸� ����\n";
		}
	}

	void release() noexcept
	{
		m_Size = 0;
		if (mp_Data != nullptr)
		{
			delete[] mp_Data;
			mp_Data = nullptr;
		}
	}

	// can throw out_of_range
	int& operator[](const size_t index_)
	{
		if (index_ < m_Size)
		{
			return mp_Data[index_];
		}

		throw std::out_of_range("�ε��� ���� �ʰ�");
	}

	size_t size() const { return m_Size; }

private:
	int* mp_Data;
	size_t m_Size;
};