#pragma once

#include <string>
#include <mutex>
#include <utility>
#include <Windows.h>

// 캐릭터의 위치정보를 수신하기 위해서 도입이 필요할 것같다.
// 1. 현재위치 2. 현재속도
// 위 두 값을 클라이언트에서 특정시간마다 1회 시도한다. (30ms정도마다 해도 되려나?)
// 위치정보값을 바탕으로 패킷 사이 간격은 예측하여 표현한다. (데드레커닝)
// 
// 그러면 위치벡터 속도벡터 수신시간을 기록하여 위치를 그리면 되나?
// 일반적인 이동이 아닌 경우에는 예측하지 않는다 (캐릭터의 이동속도가 있을테니.. 그걸로 예측)
// ex. 500ms 이내에만 빠른 가속을 얻는 스킬 같은건 따로 스킬 사용에 대한 위치반영을 하는게 맞다.
// 순간적으로 평소의 n배 속력을 내는데 패킷지연이 된다면 이상한 곳까지 갈 수도 있으니.
// 
// 해당 구조를 사용하려면
// 한 패킷의 크기를 별도로 적어줘야함. (길이마다 잘라서 사용)
// 한 패킷의 크기가 지나치게 큰 것은 확인작업이 필요함 (클라이언트가 악의적으로 적으면 어떡하지?)
// 
// 이후 Connection에 추가하고 따로 송신버퍼는 만들어 둔 뒤
// 쌓이는 패킷을 여기 저장해두고, 1회 송신이 끝나면 최대한 dequeue해서 송신버퍼에 담고 송신예약을 건다.
//
// 수신은?
// 일단 수신한 데이터는 처리할 수 있는 만큼 ReqHandler에 처리를 맡기고,
// 데이터가 잘려 실행할 수 없는 경우는 Connection에 담아두었다가 이후 수신에서 이어서 처리하자.

class RingBuffer
{
public:
	RingBuffer()
	{
		m_pData = nullptr;
	}

	~RingBuffer()
	{
		if (m_pData != nullptr)
		{
			delete[] m_pData;
		}
	}

	bool Init(const int capacity_)
	{
		if (capacity_ <= 0)
		{
			return false;
		}

		m_Capacity = capacity_;
		m_ReadPos = 0;
		m_WritePos = 0;

		if (m_pData != nullptr)
		{
			delete m_pData;
		}
		m_pData = new char[capacity_];
		ZeroMemory(m_pData, capacity_);

		return true;
	}


	bool enqueue(char* in_, int size_)
	{
		// 데이터 안담음
		if (size_ <= 0)
		{
			return false;
		}

		// 애초에 못담음
		if (size_ >= m_Capacity)
		{
			return false;
		}

		std::lock_guard<std::mutex> guard(m_mutex);

		// 남은 공간 초과
		if ((m_WritePos > m_ReadPos && (m_WritePos + size_) >= m_ReadPos + m_Capacity) ||
			(m_WritePos < m_ReadPos && (m_WritePos + size_) >= m_ReadPos))
		{
			return false;
		}

		char* source = in_;
		int leftsize = size_;

		// 저장할 데이터가 선형버퍼의 끝에 도달함 -> 분할 저장해야함 (끝부분, 처음부분으로 나눠서)
		if (m_WritePos + size_ >= m_Capacity)
		{
			int availableSpace = m_Capacity - m_WritePos;
			CopyMemory(&m_pData[m_WritePos], source, availableSpace);
			m_WritePos = 0;
			source += availableSpace;
			leftsize -= availableSpace;
		}

		CopyMemory(&m_pData[m_WritePos], source, leftsize);
		m_WritePos += leftsize;

		m_WritePos %= m_Capacity;

		return true;
	}

	// ret : peeked Data size
	int dequeue(char* out_, int maxSize_)
	{
		std::lock_guard<std::mutex> guard(m_mutex);

		if (IsEmpty())
		{
			return 0;
		}

		int remain;

		// 일반적인 경우
		if (m_WritePos > m_ReadPos)
		{
			remain = m_WritePos - m_ReadPos;
			if (remain > maxSize_)
			{
				remain = maxSize_;
			}

			CopyMemory(out_, &m_pData[m_ReadPos], remain);
			m_ReadPos += remain;
			m_ReadPos %= m_Capacity;
			return remain;
		}
		// 선형버퍼의 끝까지 읽으면 됨
		else if (m_WritePos == 0)
		{
			remain = m_Capacity - m_ReadPos;
			if (remain > maxSize_)
			{
				remain = maxSize_;
			}
			CopyMemory(out_, &m_pData[m_ReadPos], remain);
			m_ReadPos += remain;
			m_ReadPos %= m_Capacity;
			return remain;
		}
		// 선형버퍼의 끝까지 읽고 선형버퍼의 맨 앞부터 이어서 읽으면 됨
		else
		{
			remain = m_Capacity - m_ReadPos;

			if (remain > maxSize_)
			{
				remain = maxSize_;
				CopyMemory(out_, &m_pData[m_ReadPos], remain);
				m_ReadPos += remain;
				m_ReadPos %= m_Capacity;
				return remain;
			}

			CopyMemory(out_, &m_pData[m_ReadPos], remain);

			if (maxSize_ - remain < m_WritePos)
			{
				CopyMemory(out_ + remain, &m_pData[0], maxSize_ - remain);
				m_ReadPos = maxSize_ - remain;
				m_ReadPos %= m_Capacity;
				return maxSize_;
			}

			CopyMemory(out_ + remain, &m_pData[0], m_WritePos);
			m_ReadPos = m_WritePos;
			return remain + m_WritePos;
		}
	}

	bool IsEmpty() const { return m_ReadPos == m_WritePos; }
	bool IsFull() const { return (m_WritePos + 1) % m_Capacity == m_ReadPos; }
private:
	int m_ReadPos;
	int m_WritePos;

	char* m_pData;
	int m_Capacity;

	std::mutex m_mutex;
};