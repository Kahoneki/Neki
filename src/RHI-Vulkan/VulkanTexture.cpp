#include "VulkanTexture.h"

#include "VulkanDevice.h"
#include "VulkanUtils.h"

#include <Core/Utils/EnumUtils.h>

#include <stdexcept>


namespace NK
{

	VulkanTexture::VulkanTexture(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const TextureDesc& _desc)
	: ITexture(_logger, _allocator, _device, _desc, true)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::TEXTURE, "Initialising VulkanTexture\n");

		
		//m_dimension represents the dimensionality of the underlying image
		//If m_arrayTexture == true and m_dimension == TEXTURE_DIMENSION::DIM_3, this likely means there has been a misunderstanding of the parameters
		if (m_arrayTexture && m_dimension == TEXTURE_DIMENSION::DIM_3)
		{
			m_logger.Log(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TEXTURE, "_desc.arrayTexture=true, _desc.dimension=TEXTURE_DIMENSION::DIM_3. The dimension parameter represents the dimensionality of the underlying texture and an array of 3D textures is not supported. Did you mean to set _desc.dimension=TEXTURE_DIMENSION::DIM_2?\n");
			throw std::runtime_error("");
		}

		
		//Create the image
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.pNext = nullptr;
		imageInfo.flags = m_cubeMap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;

		switch (m_dimension)
		{
		case TEXTURE_DIMENSION::DIM_1: imageInfo.imageType = VK_IMAGE_TYPE_1D; break;
		case TEXTURE_DIMENSION::DIM_2: imageInfo.imageType = VK_IMAGE_TYPE_2D; break;
		case TEXTURE_DIMENSION::DIM_3: imageInfo.imageType = VK_IMAGE_TYPE_3D; break;
		}

		imageInfo.extent.width = m_size.x;
		imageInfo.extent.height = m_size.y;
		imageInfo.extent.depth = m_arrayTexture ? 1 : m_size.z;
		imageInfo.mipLevels = _desc.mipLevels;

		if (m_arrayTexture)
		{
			imageInfo.arrayLayers = (m_dimension == TEXTURE_DIMENSION::DIM_1 ? m_size.y : m_size.z);
		}
		else
		{
			imageInfo.arrayLayers = 1;
		}

		imageInfo.format = VulkanUtils::GetVulkanFormat(m_format);
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VulkanUtils::GetVulkanImageUsageFlags(m_usage);
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = VulkanUtils::GetVulkanSampleCount(m_sampleCount);

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		
		VmaAllocationInfo allocResultInfo{};
		const VkResult result{ vmaCreateImage(dynamic_cast<VulkanDevice&>(m_device).GetVMAAllocator(), &imageInfo, &allocInfo, &m_texture, &m_allocation, &allocResultInfo) };
		if (result == VK_SUCCESS)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::BUFFER, "VkBuffer initialisation and allocation successful\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::BUFFER, "VkBuffer initialisation or allocation unsuccessful. result = " + std::to_string(result) + '\n');
			throw std::runtime_error("");
		}


		m_logger.Unindent();
	}



	VulkanTexture::~VulkanTexture()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::TEXTURE, "Shutting Down VulkanTexture\n");

		
		if (!m_isOwned)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::TEXTURE, "Texture not owned by VulkanTexture class, skipping shutdown sequence\n");
			m_logger.Unindent();
			return;
		}

		if (m_texture != VK_NULL_HANDLE && m_allocation != VK_NULL_HANDLE)
		{
			vmaDestroyImage(dynamic_cast<VulkanDevice&>(m_device).GetVMAAllocator(), m_texture, m_allocation);
			m_texture = VK_NULL_HANDLE;
			m_allocation = VK_NULL_HANDLE;
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::TEXTURE, "Texture Destroyed And Allocation Freed\n");
		}
		

		m_logger.Unindent();
	}



	VulkanTexture::VulkanTexture(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const TextureDesc& _desc, VkImage _image)
	: ITexture(_logger, _allocator, _device, _desc, false)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::TEXTURE, "Initialising VulkanTexture (wrapping existing VkImage)\n");

		
		m_texture = _image;
		//m_allocation can stay uninitialised, it isn't managed by this class

		
		m_logger.Unindent();
	}
	
}