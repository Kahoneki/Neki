#pragma once
#include <RHI/IFence.h>

namespace NK
{
	class VulkanFence final : public IFence
	{
	public:
		explicit VulkanFence(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const FenceDesc& _desc);
		virtual ~VulkanFence() override;

		virtual void Wait() override;
		virtual void Reset() override;

		//Vulkan internal API (for use by other RHI-Vulkan classes)
		[[nodiscard]] inline VkFence GetFence() const { return m_fence; }


	private:
		VkFence m_fence{ VK_NULL_HANDLE };
	};
}