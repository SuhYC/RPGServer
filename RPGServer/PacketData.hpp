#pragma once

#include "Define.hpp"
#include <sstream>
#include <atomic>

const unsigned int PACKET_SIZE = 2048;

// Network IO Packet
class PacketData
{
private:
	char* m_pData;
	DWORD ioSize;

public:
	PacketData()
	{
		ioSize = 0;

		m_pData = new char[PACKET_SIZE];
	}

	virtual ~PacketData()
	{
		if (m_pData != nullptr)
		{
			Free();
		}
	}

	void Clear()
	{
		ZeroMemory(m_pData, PACKET_SIZE);
	}

	void Free()
	{
		if (m_pData == nullptr)
		{
			return;
		}

		delete m_pData;

		m_pData = nullptr;
	}

	// √ ±‚»≠
	bool Init(std::string& strData_)
	{
		if (m_pData == nullptr)
		{
			return false;
		}

		ioSize = strData_.size();
		CopyMemory(m_pData, &ioSize, sizeof(DWORD));
		CopyMemory(m_pData + sizeof(DWORD), strData_.c_str(), ioSize);

		ioSize += sizeof(DWORD);

		return true;
	}

	char* GetData() { return m_pData; }
	DWORD GetSize() const { return ioSize; }
};