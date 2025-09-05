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

		[[nodiscard]] static VkImageUsageFlags GetVulkanUsageFlags(TEXTURE_USAGE_FLAGS _flags);
		[[nodiscard]] static TEXTURE_USAGE_FLAGS GetRHIUsageFlags(VkImageUsageFlags _flags);
		[[nodiscard]] static VkFormat GetVulkanFormat(TEXTURE_FORMAT _format);
		[[nodiscard]] static TEXTURE_FORMAT GetRHIFormat(VkFormat _format);


		//Vulkan internal API (for use by other RHI-Vulkan classes)
		[[nodiscard]] VkImage GetTexture() const { return m_texture; }


	private:
		VkImage m_texture{ VK_NULL_HANDLE };
		VkDeviceMemory m_memory{ VK_NULL_HANDLE };

		//True if the lifetime of the texture is to be managed by this class (i.e. if the normal VulkanTexture constructor is called)
		//False is the lifetime of the texture is to be managed elsewhere (i.e. if the VulkanTexture object is constructed with a VkImage)
		//This flag exists mainly for the swapchain which in Vulkan, owns its images
		//^trying to destroy the images yourself (e.g. in this class' destructor) results in a crash
		bool m_isOwned;
	};
}