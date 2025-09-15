#pragma once
#include <RHI/IFence.h>

namespace NK
{
	class VulkanFence final : public IFence
	{
	public:
		explicit VulkanFence(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const FenceDesc& _desc);
		virtual ~VulkanFence();

		virtual void Wait() override;
		virtual void Reset() override;


	private:
		VkFence m_fence{ VK_NULL_HANDLE };
	};
}