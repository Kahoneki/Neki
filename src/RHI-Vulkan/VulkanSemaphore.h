#pragma once

#include <RHI/ISemaphore.h>


namespace NK
{
	
	class VulkanSemaphore final : public ISemaphore
	{
	public:
		explicit VulkanSemaphore(ILogger& _logger, IAllocator& _allocator, IDevice& _device);
		virtual ~VulkanSemaphore() override;

		//Vulkan internal API (for use by other RHI-Vulkan classes)
		[[nodiscard]] inline VkSemaphore GetSemaphore() const { return m_semaphore; }
		

	private:
		VkSemaphore m_semaphore{ VK_NULL_HANDLE };
	};

}