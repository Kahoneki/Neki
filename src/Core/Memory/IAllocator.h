#pragma once

#include <cstddef>

#if NEKI_VULKAN_SUPPORTED
	#include <vk_mem_alloc.h>
	#include <vulkan/vulkan.h>
#elif NEKI_D3D12_SUPPORTED
	#include <D3D12MemAlloc.h>
	#ifdef ERROR
		#undef ERROR //Conflicts with LOGGER_CHANNEL::ERROR
	#endif
#endif


namespace NK
{
	
	class IAllocator
	{
	public:
		virtual ~IAllocator() = default;

		//The _static flag is used by the tracking allocator to indicate that it shouldn't track the object - this can be dangerous, be careful!
		virtual void* Allocate(std::size_t _size, const char* _file, int _line, bool _static) = 0;
		//The _static flag is used by the tracking allocator to indicate that it shouldn't track the object - this can be dangerous, be careful!
		virtual void* Reallocate(void* _original, std::size_t _size, const char* _file, int line, bool _static) = 0;
		//The _static flag is used by the tracking allocator to indicate that it shouldn't track the object - this can be dangerous, be careful!
		virtual void Free(void* _ptr, bool _static) = 0;

		#if NEKI_VULKAN_SUPPORTED
			[[nodiscard]] inline const VkAllocationCallbacks* GetVulkanCallbacks() const { return &m_vulkanCallbacks; }
			[[nodiscard]] inline const VmaDeviceMemoryCallbacks* GetVMACallbacks() const { return &m_vmaCallbacks; }
		#elif NEKI_D3D12_SUPPORTED
			[[nodiscard]] inline const D3D12MA::ALLOCATION_CALLBACKS* GetD3D12MACallbacks() const { return &m_d3d12maCallbacks; }
		#endif


	protected:
		#if NEKI_VULKAN_SUPPORTED
			VkAllocationCallbacks m_vulkanCallbacks{ VK_NULL_HANDLE };
			VmaDeviceMemoryCallbacks m_vmaCallbacks{ VK_NULL_HANDLE };
		#elif NEKI_D3D12_SUPPORTED
			D3D12MA::ALLOCATION_CALLBACKS m_d3d12maCallbacks;
		#endif
	};
	
}