#pragma once

#include <cstddef>

#if NEKI_VULKAN_SUPPORTED
	#include <vulkan/vulkan.h>
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
			[[nodiscard]] virtual inline const VkAllocationCallbacks* GetVulkanCallbacks() const = 0;
		#endif


	protected:
		#if NEKI_VULKAN_SUPPORTED
			VkAllocationCallbacks m_vulkanCallbacks{ VK_NULL_HANDLE };
		#endif
	};
	
}