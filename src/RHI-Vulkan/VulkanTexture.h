#pragma once
#include "RHI/ITexture.h"

namespace NK
{
	class VulkanTexture final : public ITexture
	{
	public:
		explicit VulkanTexture(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const TextureDesc& _desc);
		virtual ~VulkanTexture() override;


	private:
		[[nodiscard]] VkImageUsageFlags GetVulkanUsageFlags() const;
		[[nodiscard]] VkFormat GetVulkanFormat() const;
		
		
		VkImage m_texture{ VK_NULL_HANDLE };
		VkDeviceMemory m_memory{ VK_NULL_HANDLE };
	};
}
