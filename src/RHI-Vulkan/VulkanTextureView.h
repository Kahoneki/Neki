#pragma once

#include "VulkanDevice.h"

#include <RHI/ITextureView.h>


namespace NK
{

	class VulkanTextureView final : public ITextureView
	{
	public:
		//Use this constructor if you want a free list allocator to be used for allocation (will be the case for shader-accessible view types)
		explicit VulkanTextureView(ILogger& _logger, IAllocator& _allocator, IDevice& _device, ITexture* _texture, const TextureViewDesc& _desc, VkDescriptorSet _descriptorSet, FreeListAllocator* _freeListAllocator);

		//Use this constructor if just making a raw image view, i.e.: this constructor will not write to a descriptor set (will be the case for non-shader-accessible view types)
		explicit VulkanTextureView(ILogger& _logger, IAllocator& _allocator, IDevice& _device, ITexture* _texture, const TextureViewDesc& _desc);

		virtual ~VulkanTextureView() override;

		//Vulkan internal API (for use by other RHI-Vulkan classes)
		[[nodiscard]] inline VkImageView GetImageView() const { return m_imageView; }
		[[nodiscard]] inline VkRect2D GetRenderArea() const { return m_renderArea; }
		
		[[nodiscard]] static VkImageViewType GetVulkanImageViewType(TEXTURE_VIEW_DIMENSION _dimension);


	private:
		//Vulkan handles
		VkImageView m_imageView{ VK_NULL_HANDLE };

		VkRect2D m_renderArea;
	};

}