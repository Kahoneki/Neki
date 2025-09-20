#pragma once
#include <RHI/IQueue.h>

namespace NK
{
	class VulkanQueue final : public IQueue
	{
		friend class VulkanDevice;
		
	public:
		explicit VulkanQueue(ILogger& _logger, IDevice& _device, const QueueDesc& _desc, FreeListAllocator& _queueIndexAllocator);
		virtual ~VulkanQueue() override;

		virtual void Submit(ICommandBuffer* _cmdBuffer, ISemaphore* _waitSemaphore, ISemaphore* _signalSemaphore, IFence* _signalFence) override;
		virtual void WaitIdle() override;

		//Vulkan internal API (for use by other RHI-Vulkan classes)
		[[nodiscard]] inline VkQueue GetQueue() const { return m_queue; }


	private:
		//Dependency injections
		FreeListAllocator& m_queueIndexAllocator;
		
		VkQueue m_queue{ VK_NULL_HANDLE };
		std::uint32_t m_queueIndex{ FreeListAllocator::INVALID_INDEX };
	};
}