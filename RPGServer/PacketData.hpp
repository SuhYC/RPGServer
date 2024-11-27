#pragma once

#include "Define.hpp"
#include <sstream>
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

public:
	PacketData()
	{
		m_SessionIndex = 0;
		m_pBlock = nullptr;
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
	void Init(const unsigned short sessionIndex_, std::string& strData_)
	{
		m_SessionIndex = sessionIndex_;

		std::ostringstream oss;
		oss << "[" << strData_.size() << "]" << strData_;

		std::string newstr = oss.str();

		char* msg = nullptr;

		try
		{
			msg = new char[newstr.size()];
		}
		catch (const std::bad_alloc&)
		{
			std::cerr << "ReqHandler::MakePacket : 메모리 부족\n";
			return;
		}

		CopyMemory(msg, newstr.c_str(), newstr.size());

		m_pBlock = new Block(msg, newstr.size());
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
};