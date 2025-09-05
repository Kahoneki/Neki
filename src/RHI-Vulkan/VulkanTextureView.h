#pragma once
#include <RHI/ITextureView.h>
#include "VulkanDevice.h"

namespace NK
{

	class VulkanTextureView final : public ITextureView
	{
	public:
		//If creating a texture view for a shader-accessible texture (_desc.type = SHADER_RESOURCE or UNORDERED_ACCESS, pass a free list allocator and descriptor set for the bindless update)
		//Otherwise (if _desc.type = RENDER_TARGET, DEPTH, or STENCIL), leave _freeListAllocator and _descriptorSet uninitialised
		explicit VulkanTextureView(ILogger& _logger, IAllocator& _allocator, IDevice& _device, ITexture* _texture, const TextureViewDesc& _desc, FreeListAllocator* _freeListAllocator = nullptr, VkDescriptorSet _descriptorSet = VK_NULL_HANDLE);

		virtual ~VulkanTextureView() override;


	private:
		//Vulkan handles
		VkImageView m_imageView{ VK_NULL_HANDLE };
	};

}