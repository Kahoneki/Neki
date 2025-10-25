#pragma once

#include "RHI/ITexture.h"


namespace NK
{
	
	class VulkanTexture final : public ITexture
	{
	public:
		explicit VulkanTexture(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const TextureDesc& _desc);
		//For wrapping an existing VkImage
		explicit VulkanTexture(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const TextureDesc& _desc, VkImage _image);

		virtual ~VulkanTexture() override;

		//Vulkan internal API (for use by other RHI-Vulkan classes)
		[[nodiscard]] VkImage GetTexture() const { return m_texture; }


	private:
		VkImage m_texture{ VK_NULL_HANDLE };
		VmaAllocation m_allocation{ VK_NULL_HANDLE };
	};

}