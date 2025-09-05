#include "VulkanTextureView.h"

#include "VulkanTexture.h"
#include <stdexcept>

namespace NK
{

	VulkanTextureView::VulkanTextureView(ILogger& _logger, IAllocator& _allocator, IDevice& _device, ITexture* _texture, const TextureViewDesc& _desc, FreeListAllocator* _freeListAllocator, VkDescriptorSet _descriptorSet)
	: ITextureView(_logger, _allocator, _device, _texture, _desc, _freeListAllocator)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::TEXTURE_VIEW, "Creating texture view\n");

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = dynamic_cast<VulkanTexture*>(_texture)->GetTexture();

		//todo: add array support
		switch (m_dimension)
		{
		case TEXTURE_DIMENSION::DIM_1:	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_1D; break;
		case TEXTURE_DIMENSION::DIM_2:	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; break;
		case TEXTURE_DIMENSION::DIM_3:	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_3D; break;
		}
		
		viewInfo.format = VulkanTexture::GetVulkanFormat(m_format);

		
		switch (m_type)
		{
		case TEXTURE_VIEW_TYPE::RENDER_TARGET:
		case TEXTURE_VIEW_TYPE::SHADER_READ_ONLY:
		case TEXTURE_VIEW_TYPE::SHADER_READ_WRITE:
		{
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			break;
		}
			
		case TEXTURE_VIEW_TYPE::DEPTH:		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; break;
		case TEXTURE_VIEW_TYPE::STENCIL:	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT; break;
		}

		
		//todo: add mip support
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;

		//todo: add array support
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		
		const VkResult result{ vkCreateImageView(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), &viewInfo, m_allocator.GetVulkanCallbacks(), &m_imageView) };
		if (result == VK_SUCCESS)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::TEXTURE_VIEW, "Texture view successfully created\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TEXTURE_VIEW, "Failed to create texture view - result = " + std::to_string(result) + "\n");
			throw std::runtime_error("");
		}


		//If view is a shader resource, perform bindless update
		if (m_type == TEXTURE_VIEW_TYPE::SHADER_READ_ONLY || m_type == TEXTURE_VIEW_TYPE::SHADER_READ_WRITE)
		{

			if (_descriptorSet == VK_NULL_HANDLE)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TEXTURE_VIEW, "Shader-accessible texture view requested (type = " + std::to_string(std::to_underlying(m_type)) + "), but _descriptorSet wasn't set or the descriptor set passed in was VK_NULL_HANDLE\n");
				throw std::runtime_error("");
			}

			if (m_resourceIndexAllocator == nullptr)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TEXTURE_VIEW, "Shader-accessible texture view requested (type = " + std::to_string(std::to_underlying(m_type)) + "), but _freeListAllocator wasn't set or the allocator passed in was nullptr\n");
				throw std::runtime_error("");
			}
			
			m_resourceIndex = m_resourceIndexAllocator->Allocate();

			//Convert rhi view type to vulkan descriptor type
			VkDescriptorType descriptorType;
			switch (_desc.type)
			{
			case TEXTURE_VIEW_TYPE::SHADER_READ_ONLY:
				descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				break;

			case TEXTURE_VIEW_TYPE::SHADER_READ_WRITE:
				descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				break;

			default:
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TEXTURE_VIEW, "Default case reached when determining vulkan descriptor type\n");
				throw std::runtime_error("");
			}
			
			//Populate descriptor info
			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageView = m_imageView;
			imageInfo.imageLayout = m_type == TEXTURE_VIEW_TYPE::SHADER_READ_ONLY ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;

			//Populate write info
			VkWriteDescriptorSet writeInfo{};
			writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeInfo.dstSet = _descriptorSet;
			writeInfo.dstBinding = 0; //All bindless resource views are in binding point 0
			writeInfo.dstArrayElement = m_resourceIndex;
			writeInfo.descriptorCount = 1;
			writeInfo.descriptorType = descriptorType;
			writeInfo.pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), 1, &writeInfo, 0, nullptr);
		}
		else
		{
			//Descriptor set and free list allocator shouldn't be set
			if (_descriptorSet != VK_NULL_HANDLE)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::TEXTURE_VIEW, "Descriptor set passed into constructor, but _desc.type is not shader-accessible. _descriptorSet is ignored in this case and does not need to be populated in constructor parameters\n");
			}
			if (m_resourceIndexAllocator != nullptr)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::TEXTURE_VIEW, "Free list allocator passed into constructor, but _desc.type is not shader-accessible. _freeListAllocator is ignored in this case and does not need to be populated in constructor parameters\n");
			}
		}
		
		
		m_logger.Unindent();
	}



	VulkanTextureView::~VulkanTextureView()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::TEXTURE_VIEW, "Shutting Down VulkanTextureView\n");

		if (m_imageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), m_imageView, m_allocator.GetVulkanCallbacks());
			m_imageView = VK_NULL_HANDLE;
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::TEXTURE_VIEW, "Texture View Destroyed\n");
		}
		
		if (m_resourceIndex != FreeListAllocator::INVALID_INDEX)
		{
			m_resourceIndexAllocator->Free(m_resourceIndex);
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::TEXTURE_VIEW, "Resource Index Freed\n");
			m_resourceIndex = FreeListAllocator::INVALID_INDEX;
		}
		
		m_logger.Unindent();
	}
	
}