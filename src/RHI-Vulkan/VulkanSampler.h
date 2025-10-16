#pragma once

#include <RHI/ISampler.h>


namespace NK
{
	
	class VulkanSampler final : public ISampler
	{
	public:
		explicit VulkanSampler(ILogger& _logger, IAllocator& _allocator, FreeListAllocator& _freeListAllocator, IDevice& _device, const SamplerDesc& _desc, VkDescriptorSet _descriptorSet);
		virtual ~VulkanSampler() override;

		//Vulkan internal API (for use by other RHI-Vulkan classes)
		[[nodiscard]] inline VkSampler GetSampler() const { return m_sampler; }
		

	private:
		VkSampler m_sampler{ VK_NULL_HANDLE };
	};

}