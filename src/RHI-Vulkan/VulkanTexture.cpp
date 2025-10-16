#include "VulkanTexture.h"

#include "VulkanDevice.h"
#include "VulkanUtils.h"

#include <Core/Utils/EnumUtils.h>
#include <Core/Utils/FormatUtils.h>

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
		imageInfo.mipLevels = 1;

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
		VkResult result{ vkCreateImage(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), &imageInfo, m_allocator.GetVulkanCallbacks(), &m_texture) };
		if (result == VK_SUCCESS)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::TEXTURE, "VkImage initialisation successful\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TEXTURE, "VkImage initialisation unsuccessful. result = " + std::to_string(result) + '\n');
			throw std::runtime_error("");
		}


		//Search for a memory type that satisfies image's internal requirements and that is DEVICE_LOCAL
		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), m_texture, &memRequirements);

		VkPhysicalDeviceMemoryProperties physicalDeviceMemProps;
		vkGetPhysicalDeviceMemoryProperties(dynamic_cast<VulkanDevice&>(m_device).GetPhysicalDevice(), &physicalDeviceMemProps);
		std::uint32_t memTypeIndex{ UINT32_MAX };
		for (std::uint32_t i{ 0 }; i < physicalDeviceMemProps.memoryTypeCount; ++i)
		{
			//Check if memory type is allowed for the buffer
			if (!(memRequirements.memoryTypeBits & (1 << i))) { continue; }
			if (!(physicalDeviceMemProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) { continue; }

			//This memory type is allowed
			memTypeIndex = i;
			break;
		}
		if (memTypeIndex == UINT32_MAX)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TEXTURE, "No suitable memory type could be found\n");
			throw std::runtime_error("");
		}


		//Allocate memory for the image
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = memTypeIndex;
		result = vkAllocateMemory(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), &allocInfo, m_allocator.GetVulkanCallbacks(), &m_memory);
		if (result == VK_SUCCESS)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::TEXTURE, "VkDeviceMemory allocation successful (" + FormatUtils::GetSizeString(memRequirements.size) + ")\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TEXTURE, "VkDeviceMemory allocation unsuccessful. result = " + std::to_string(result) + '\n');
			throw std::runtime_error("");
		}


		//Bind the allocated memory region to the image
		result = vkBindImageMemory(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), m_texture, m_memory, 0);
		if (result == VK_SUCCESS)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::TEXTURE, "Memory binding successful\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TEXTURE, "Memory binding unsuccessful. result = " + std::to_string(result) + '\n');
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
		
		if (m_memory != VK_NULL_HANDLE)
		{
			vkFreeMemory(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), m_memory, m_allocator.GetVulkanCallbacks());
			m_memory = VK_NULL_HANDLE;
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::TEXTURE, "Texture Memory Freed\n");
		}
		
		if (m_texture != VK_NULL_HANDLE)
		{
			vkDestroyImage(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), m_texture, m_allocator.GetVulkanCallbacks());
			m_texture = VK_NULL_HANDLE;
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::TEXTURE, "Texture Destroyed\n");
		}

		m_logger.Unindent();
	}



	VulkanTexture::VulkanTexture(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const TextureDesc& _desc, VkImage _image)
	: ITexture(_logger, _allocator, _device, _desc, false)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::TEXTURE, "Initialising VulkanTexture (wrapping existing VkImage)\n");

		m_texture = _image;
		//m_memory can stay uninitialised, it isn't managed by this class
		
		m_logger.Unindent();
	}
	
}