#include "VulkanTextureView.h"

#include "VulkanTexture.h"
#include "VulkanUtils.h"

#include <stdexcept>


namespace NK
{

	VulkanTextureView::VulkanTextureView(ILogger& _logger, IAllocator& _allocator, IDevice& _device, ITexture* _texture, const TextureViewDesc& _desc, VkDescriptorSet _descriptorSet, FreeListAllocator* _freeListAllocator)
	: ITextureView(_logger, _allocator, _device, _texture, _desc, _freeListAllocator, true)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::TEXTURE_VIEW, "Creating Texture View\n");


		//This constructor logically requires shader-accessible view type
		//Todo: maybe look into whether it makes sense to lift this requirement but for now I can't think of a scenario where it would make sense
		if (_desc.type == TEXTURE_VIEW_TYPE::RENDER_TARGET || _desc.type == TEXTURE_VIEW_TYPE::DEPTH_STENCIL)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TEXTURE_VIEW, "This constructor is only to be used for shader-accessible view types. This requirement will maybe be lifted in a future version - if there is a valid reason for needing this constructor in this instance, please make a GitHub issue on the topic.\n");
			throw std::runtime_error("");
		}


		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = dynamic_cast<VulkanTexture*>(_texture)->GetTexture();
		viewInfo.viewType = VulkanUtils::GetVulkanImageViewType(m_dimension);
		viewInfo.format = VulkanUtils::GetVulkanFormat(m_format);
		viewInfo.subresourceRange.aspectMask = VulkanUtils::GetVulkanAspectFromRHIFormat(m_format);
		
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = _texture->GetMipLevels();

		viewInfo.subresourceRange.baseArrayLayer = m_baseArrayLayer;
		viewInfo.subresourceRange.layerCount = m_arrayLayerCount;

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


		//Perform bindless update
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


		//Populate m_renderArea if _texture is 2D
		if (m_dimension == TEXTURE_VIEW_DIMENSION::DIM_2)
		{
			m_renderArea.offset = { 0, 0 };
			m_renderArea.extent = { static_cast<std::uint32_t>(_texture->GetSize().x), static_cast<std::uint32_t>(_texture->GetSize().y) };
		}
		
		
		m_logger.Unindent();
	}



	VulkanTextureView::VulkanTextureView(ILogger& _logger, IAllocator& _allocator, IDevice& _device, ITexture* _texture, const TextureViewDesc& _desc)
	: ITextureView(_logger, _allocator, _device, _texture, _desc, nullptr, false)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::TEXTURE_VIEW, "Creating Texture View\n");


		//This constructor logically requires non-shader-accessible view type
		//Todo: maybe look into whether it makes sense to lift this requirement but for now I can't think of a scenario where it would make sense
		if (_desc.type == TEXTURE_VIEW_TYPE::SHADER_READ_ONLY || _desc.type == TEXTURE_VIEW_TYPE::SHADER_READ_WRITE)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TEXTURE_VIEW, "This constructor is only to be used for non-shader-accessible view types. This requirement will maybe be lifted in a future version - if there is a valid reason for needing this constructor in this instance, please make a GitHub issue on the topic.\n");
			throw std::runtime_error("");
		}


		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = dynamic_cast<VulkanTexture*>(_texture)->GetTexture();
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; //todo: look into possible scenarios where you would want a non-2d rtv/dsv?
		viewInfo.format = VulkanUtils::GetVulkanFormat(m_format);
		switch (m_type)
		{
		case TEXTURE_VIEW_TYPE::RENDER_TARGET:	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; break;
		case TEXTURE_VIEW_TYPE::DEPTH:			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; break;
		case TEXTURE_VIEW_TYPE::DEPTH_STENCIL:	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT; break;
		default:
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TEXTURE_VIEW, "Default case reached for m_type switch case in VulkanTextureView constructor - this suggests an internal error rather than a user-caused one. Please make a GitHub issue on the topic.\n");
			throw std::runtime_error("");
		}
		}

		//todo: add mip support
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;

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


		//Populate m_renderArea if _texture is 2D / CUBE
		if (m_dimension == TEXTURE_VIEW_DIMENSION::DIM_2 || m_dimension == TEXTURE_VIEW_DIMENSION::DIM_2D_ARRAY || m_dimension == TEXTURE_VIEW_DIMENSION::DIM_CUBE)
		{
			m_renderArea.offset = { 0, 0 };
			m_renderArea.extent = { static_cast<std::uint32_t>(_texture->GetSize().x), static_cast<std::uint32_t>(_texture->GetSize().y) };
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
		
		if (m_resourceIndexAllocator && m_resourceIndex != FreeListAllocator::INVALID_INDEX)
		{
			m_resourceIndexAllocator->Free(m_resourceIndex);
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::TEXTURE_VIEW, "Resource Index Freed\n");
			m_resourceIndex = FreeListAllocator::INVALID_INDEX;
		}
		

		m_logger.Unindent();
	}



	VkImageViewType VulkanTextureView::GetVulkanImageViewType(TEXTURE_VIEW_DIMENSION _dimension)
	{
		switch (_dimension)
		{
		case TEXTURE_VIEW_DIMENSION::DIM_1:				return VK_IMAGE_VIEW_TYPE_1D;
		case TEXTURE_VIEW_DIMENSION::DIM_2:				return VK_IMAGE_VIEW_TYPE_2D;
		case TEXTURE_VIEW_DIMENSION::DIM_3:				return VK_IMAGE_VIEW_TYPE_3D;
		case TEXTURE_VIEW_DIMENSION::DIM_CUBE:			return VK_IMAGE_VIEW_TYPE_CUBE;
		case TEXTURE_VIEW_DIMENSION::DIM_1D_ARRAY:		return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
		case TEXTURE_VIEW_DIMENSION::DIM_2D_ARRAY:		return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		case TEXTURE_VIEW_DIMENSION::DIM_CUBE_ARRAY:	return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;

		default:
		{
			throw std::runtime_error("GetVulkanImageViewType() default case reached. Dimension = " + std::to_string(std::to_underlying(_dimension)));
		}
		}
	}

}