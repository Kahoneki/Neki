#pragma once

#include <cstdint>
#include <vector>
#include <Core/Utils/Serialisation/Serialisation.h>


namespace NK
{
	
	class FreeListAllocator
	{
		friend class cereal::access;
		
		
	public:
		explicit FreeListAllocator(std::size_t _maxActiveAllocations)
		: m_maxActiveAllocations(_maxActiveAllocations), m_nextFreeIndex(0) {}

		FreeListAllocator() = delete;
		
		~FreeListAllocator() = default;
		
		[[nodiscard]] std::uint32_t Allocate();
		void Free(std::uint32_t _val);
		
		template<class Archive>
		void serialize(Archive& archive)
		{
			archive(m_maxActiveAllocations, m_nextFreeIndex, m_freeList);
		}
		
		
		static constexpr std::uint32_t INVALID_INDEX{ UINT32_MAX };
		

	private:
		std::size_t m_maxActiveAllocations;
		std::uint32_t m_nextFreeIndex;
		std::vector<std::uint32_t> m_freeList;
	};
	
}