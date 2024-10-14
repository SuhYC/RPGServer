#pragma once
#include <stdexcept>

// DB에서 받은 데이터를 저장할 클래스.
// 이후 Json화 하여 전달 예정
// 캐릭터의 코드만 받아온다. int타입.
// 갯수를 제한할까? 제한하면 일일히 동적할당할 이유가 없다
class CharList
{
public:
	void Init(const size_t count_)
	{
		m_Size = count_;
		if (mp_Data != nullptr)
		{
			delete[] mp_Data;
		}
		mp_Data = new int[count_];
	}

	void release()
	{
		m_Size = 0;
		delete[] mp_Data;
		mp_Data = nullptr;
	}

	int& operator[](const size_t index_)
	{
		if (index_ < m_Size)
		{
			return mp_Data[index_];
		}

		throw std::out_of_range("인덱스 범위 초과");
	}

	size_t size() const { return m_Size; }

private:
	int* mp_Data;
	size_t m_Size;
};