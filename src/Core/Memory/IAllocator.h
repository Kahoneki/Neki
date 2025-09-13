#pragma once
#if NEKI_VULKAN_SUPPORTED
	#include <vulkan/vulkan.h>
#endif
#include <cstdint>

namespace NK
{
	enum class ALLOCATOR_TYPE
	{
		TRACKING,
		TRACKING_VERBOSE,
		TRACKING_VERBOSE_INCLUDE_VULKAN,
	};


	class IAllocator
	{
	public:
		virtual ~IAllocator() = default;

		virtual void* Allocate(std::size_t _size, const char* _file, int _line) = 0;
		virtual void* Reallocate(void* _original, std::size_t _size, const char* _file, int line) = 0;
		virtual void Free(void* _ptr) = 0;

		#if NEKI_VULKAN_SUPPORTED
			[[nodiscard]] virtual inline const VkAllocationCallbacks* GetVulkanCallbacks() const = 0;
		#endif


	protected:
		#if NEKI_VULKAN_SUPPORTED
			VkAllocationCallbacks m_vulkanCallbacks{ VK_NULL_HANDLE };
		#endif
	};
}