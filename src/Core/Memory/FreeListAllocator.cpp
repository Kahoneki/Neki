#include "FreeListAllocator.h"

namespace NK
{
	
	std::uint32_t FreeListAllocator::Allocate()
	{
		if (!m_freeList.empty())
		{
			const std::uint32_t index{ m_freeList.back() };
			m_freeList.pop_back();
			return index;
		}

		return m_nextFreeIndex++;
	}



	void FreeListAllocator::Free(std::uint32_t _val)
	{
		m_freeList.push_back(_val);
	}

}