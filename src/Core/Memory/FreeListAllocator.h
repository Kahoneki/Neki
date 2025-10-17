#pragma once

#include <cstdint>
#include <vector>


namespace NK
{
	
	class FreeListAllocator
	{
	public:
		explicit FreeListAllocator(std::size_t _maxActiveAllocations)
		: m_maxActiveAllocations(_maxActiveAllocations), m_nextFreeIndex(0) {}

		FreeListAllocator() = delete;
		
		~FreeListAllocator() = default;
		
		[[nodiscard]] std::uint32_t Allocate();
		void Free(std::uint32_t _val);
		
		static constexpr std::size_t INVALID_INDEX{ UINT32_MAX };
		

	private:
		std::size_t m_maxActiveAllocations;
		std::uint32_t m_nextFreeIndex;
		std::vector<std::uint32_t> m_freeList;
	};
	
}