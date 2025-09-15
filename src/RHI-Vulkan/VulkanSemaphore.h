#pragma once
#include <RHI/ISemaphore.h>

namespace NK
{
	class VulkanSemaphore final : public ISemaphore
	{
	public:
		explicit VulkanSemaphore(ILogger& _logger, IAllocator& _allocator, IDevice& _device);
		virtual ~VulkanSemaphore() override;


	private:
		VkSemaphore m_semaphore{ VK_NULL_HANDLE };
	};
}