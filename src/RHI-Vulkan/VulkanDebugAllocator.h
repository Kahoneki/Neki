#pragma once

#include <mutex>
#include <unordered_map>
#include <vulkan/vulkan.h>

namespace NK
{
	enum class VULKAN_ALLOCATOR_TYPE
	{
		DEFAULT, //The built-in vulkan allocator
		DEBUG, //VKDebugAllocator with verbose=false
		DEBUG_VERBOSE, //VKDebugAllocator with verbose=true
	};


	class VulkanDebugAllocator
	{
	public:
		explicit VulkanDebugAllocator(VULKAN_ALLOCATOR_TYPE _type);
		~VulkanDebugAllocator();

		[[nodiscard]] const VkAllocationCallbacks* GetCallbacks() const
		{
			//If type is set to default, return nullptr - vulkan interprets this as the built-in allocator
			return (m_type == VULKAN_ALLOCATOR_TYPE::DEFAULT) ? nullptr : &m_callbacks;
		}
		
		
	private:
		static void* VKAPI_CALL Allocation(void* _pUserData, std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope);
		static void* VKAPI_CALL Reallocation(void* _pUserData, void* _pOriginal, std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope);
		static void VKAPI_CALL Free(void* _pUserData, void* _pMemory);
		//todo: maybe add internal allocation notification callbacks ?

		void* AllocationImpl(std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope);
		void* ReallocationImpl(void* _pOriginal, std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope);
		void FreeImpl(void* _pMemory);
		
		std::unordered_map<void*, std::size_t> m_allocationSizes; //Stores the sizes of all allocations (required for reallocation)
		std::mutex m_allocationSizesMtx;
		
		VkAllocationCallbacks m_callbacks;
		const VULKAN_ALLOCATOR_TYPE m_type;
	};
}