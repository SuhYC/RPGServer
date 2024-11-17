#pragma once

#include "Define.hpp"
#include <atomic>

// Network IO Packet
class PacketData
{
private:
	class Block
	{
	public:
		Block(char* pData_, const ULONG dataSize_) : dataSize(dataSize_)
		{
			refCount.store(1);
			pData = pData_;
		}
		virtual ~Block()
		{
			if (pData != nullptr)
			{
				delete[] pData;
			}
		}

		void PlusCount()
		{
			refCount++;
		}
		void MinusCount()
		{
			refCount--;
		}

		std::atomic<int> refCount;
		char* pData;
		const ULONG dataSize;
	};
	unsigned short m_SessionIndex;
	Block* m_pBlock;
	stOverlappedEx m_Overlapped;

public:
	PacketData()
	{
		m_SessionIndex = 0;
		m_pBlock = nullptr;
		m_Overlapped = { 0 };
	}

	virtual ~PacketData()
	{
		if (m_pBlock != nullptr)
		{
			Clear();
		}
	}

	void Clear()
	{
		if (m_pBlock == nullptr)
		{
			return;
		}

		if (m_pBlock->refCount.load() == 1)
		{
			delete m_pBlock;
		}
		else
		{
			m_pBlock->MinusCount();
		}

		m_pBlock = nullptr;
	}

	// 초기화
	void Init(const unsigned short sessionIndex_, char* pData_, const size_t dataSize_)
	{
		m_SessionIndex = sessionIndex_;
		char* msg = nullptr;

		try
		{
			msg = new char[dataSize_ + 1];
		}
		catch (const std::bad_alloc&)
		{
			std::cerr << "PacketData::Init : 메모리 부족\n";
			return;
		}

		CopyMemory(msg, pData_, dataSize_);
		msg[dataSize_] = NULL;

		m_pBlock = new Block(msg, dataSize_);
	}

	// 초기화
	void Init(const unsigned short sessionIndex_, std::string& strData_)
	{
		m_SessionIndex = sessionIndex_;
		char* msg = nullptr;

		try
		{
			msg = new char[strData_.size() + 1];
		}
		catch (const std::bad_alloc&)
		{
			std::cerr << "ReqHandler::MakePacket : 메모리 부족\n";
			return;
		}

		CopyMemory(msg, strData_.c_str(), strData_.size());
		msg[strData_.size()] = NULL;

		m_pBlock = new Block(msg, strData_.size());
	}

	// 복사
	void Init(const PacketData* other_, const unsigned short sessionIndex_)
	{
		m_SessionIndex = sessionIndex_;
		other_->GetBlock()->PlusCount();
		m_pBlock = other_->GetBlock();
	}

	Block* GetBlock() const { return m_pBlock; }
	const unsigned short GetClient() const { return m_SessionIndex; }

	void SetOverlapped()
	{
		ZeroMemory(&m_Overlapped, sizeof(stOverlappedEx));

		m_Overlapped.m_wsaBuf.len = m_pBlock->dataSize;
		m_Overlapped.m_wsaBuf.buf = m_pBlock->pData;
		m_Overlapped.m_eOperation = eIOOperation::SEND;
		m_Overlapped.m_userIndex = m_SessionIndex;
	}

	stOverlappedEx& GetOverlapped() { return m_Overlapped; }
};