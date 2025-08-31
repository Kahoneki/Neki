#pragma once
#include <cstdint>
#include <vector>

namespace NK
{
	class FreeListAllocator
	{
	public:
		FreeListAllocator() = default;
		~FreeListAllocator() = default;
		
		[[nodiscard]] std::uint32_t Allocate();
		void Free(std::uint32_t _val);


	private:
		std::uint32_t m_nextFreeIndex{ 0 };
		std::vector<std::uint32_t> m_freeList;
	};
}