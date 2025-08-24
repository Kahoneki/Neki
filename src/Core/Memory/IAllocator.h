#pragma once
#include <vulkan/vulkan.h>

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

		[[nodiscard]] virtual inline const VkAllocationCallbacks* GetVulkanCallbacks() const = 0;

	protected:
		VkAllocationCallbacks m_vulkanCallbacks{ VK_NULL_HANDLE };
	};
}