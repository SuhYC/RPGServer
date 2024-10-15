#pragma once

#include <concurrent_queue.h>
#include "Define.hpp"

template <typename T>
class MemoryPool
{
public:
	MemoryPool(size_t size_)
	{
		for (size_t i = 0; i < size_; i++)
		{
			q.push(new T());
		}
	}

	~MemoryPool()
	{
		T* tmp = nullptr;
		while (q.try_pop(tmp))
		{
			delete tmp;
		}
	}

	T* Allocate()
	{
		T* ret = nullptr;
		if (q.try_pop(ret))
		{
			return ret;
		}

		ret = new T();

		return ret;
	}

	void Deallocate(T* pT_)
	{
		q.push(pT_);
		return;
	}

private:
	Concurrency::concurrent_queue<T*> q;
};