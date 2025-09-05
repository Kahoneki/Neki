#include "VulkanTexture.h"

#include <stdexcept>
#include "VulkanDevice.h"
#include "Core/Utils/EnumUtils.h"
#include "Core/Utils/FormatUtils.h"

namespace NK
{

	VulkanTexture::VulkanTexture(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const TextureDesc& _desc)
	: ITexture(_logger, _allocator, _device, _desc)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::TEXTURE, "Initialising VulkanTexture\n");

		m_isOwned = true;

		
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
		imageInfo.flags = 0;

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
			if (m_dimension == TEXTURE_DIMENSION::DIM_1) { imageInfo.arrayLayers = m_size.y; }
			else { imageInfo.arrayLayers = m_size.z; }
		}
		else
		{
			imageInfo.arrayLayers = 1;
		}

		imageInfo.format = GetVulkanFormat(m_format);
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = GetVulkanUsageFlags(m_usage);
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
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
	: ITexture(_logger, _allocator, _device, _desc)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::TEXTURE, "Initialising VulkanTexture (wrapping existing VkImage)\n");

		m_isOwned = false;
		m_texture = _image;
		//m_memory can stay uninitialised, it isn't managed by this class
		
		m_logger.Unindent();
	}



	VkImageUsageFlags VulkanTexture::GetVulkanUsageFlags(TEXTURE_USAGE_FLAGS _flags)
	{
		VkImageUsageFlags vkFlags{ 0 };
		
		if (EnumUtils::Contains(_flags, TEXTURE_USAGE_FLAGS::TRANSFER_SRC_BIT))			{ vkFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT; }
		if (EnumUtils::Contains(_flags, TEXTURE_USAGE_FLAGS::TRANSFER_DST_BIT))			{ vkFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT; }
		if (EnumUtils::Contains(_flags, TEXTURE_USAGE_FLAGS::READ_ONLY))				{ vkFlags |= VK_IMAGE_USAGE_SAMPLED_BIT; }
		if (EnumUtils::Contains(_flags, TEXTURE_USAGE_FLAGS::READ_WRITE))				{ vkFlags |= VK_IMAGE_USAGE_STORAGE_BIT; }
		if (EnumUtils::Contains(_flags, TEXTURE_USAGE_FLAGS::COLOUR_ATTACHMENT))		{ vkFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; }
		if (EnumUtils::Contains(_flags, TEXTURE_USAGE_FLAGS::DEPTH_STENCIL_ATTACHMENT))	{ vkFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT; }

		return vkFlags;
	}



	TEXTURE_USAGE_FLAGS VulkanTexture::GetRHIUsageFlags(VkImageUsageFlags _flags)
	{
		TEXTURE_USAGE_FLAGS rhiFlags{ 0 };

		if (_flags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)			{ rhiFlags |= TEXTURE_USAGE_FLAGS::TRANSFER_SRC_BIT; }
		if (_flags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)			{ rhiFlags |= TEXTURE_USAGE_FLAGS::TRANSFER_DST_BIT; }
		if (_flags & VK_IMAGE_USAGE_SAMPLED_BIT)				{ rhiFlags |= TEXTURE_USAGE_FLAGS::READ_ONLY; }
		if (_flags & VK_IMAGE_USAGE_STORAGE_BIT)				{ rhiFlags |= TEXTURE_USAGE_FLAGS::READ_WRITE; }
		if (_flags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)		{ rhiFlags |= TEXTURE_USAGE_FLAGS::COLOUR_ATTACHMENT; }
		if (_flags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)	{ rhiFlags |= TEXTURE_USAGE_FLAGS::DEPTH_STENCIL_ATTACHMENT; }

		return rhiFlags;
	}



	VkFormat VulkanTexture::GetVulkanFormat(TEXTURE_FORMAT _format)
	{
		switch (_format)
		{
		case TEXTURE_FORMAT::UNDEFINED:					return VK_FORMAT_UNDEFINED;
			
		case TEXTURE_FORMAT::R8_UNORM:					return VK_FORMAT_R8_UNORM;
		case TEXTURE_FORMAT::R8G8_UNORM:				return VK_FORMAT_R8G8_UNORM;
		case TEXTURE_FORMAT::R8G8B8A8_UNORM:			return VK_FORMAT_R8G8B8A8_UNORM;
		case TEXTURE_FORMAT::R8G8B8A8_SRGB:				return VK_FORMAT_R8G8B8A8_SRGB;
		case TEXTURE_FORMAT::B8G8R8A8_UNORM:			return VK_FORMAT_B8G8R8A8_UNORM;
		case TEXTURE_FORMAT::B8G8R8A8_SRGB:				return VK_FORMAT_B8G8R8A8_SRGB;

		case TEXTURE_FORMAT::R16_SFLOAT:				return VK_FORMAT_R16_SFLOAT;
		case TEXTURE_FORMAT::R16G16_SFLOAT:				return VK_FORMAT_R16G16_SFLOAT;
		case TEXTURE_FORMAT::R16G16B16A16_SFLOAT:		return VK_FORMAT_R16G16B16A16_SFLOAT;

		case TEXTURE_FORMAT::R32_SFLOAT:				return VK_FORMAT_R32_SFLOAT;
		case TEXTURE_FORMAT::R32G32_SFLOAT:				return VK_FORMAT_R32G32_SFLOAT;
		case TEXTURE_FORMAT::R32G32B32A32_SFLOAT:		return VK_FORMAT_R32G32B32A32_SFLOAT;

		case TEXTURE_FORMAT::B10G11R11_UFLOAT_PACK32:	return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
		case TEXTURE_FORMAT::R10G10B10A2_UNORM:			return VK_FORMAT_A2R10G10B10_UNORM_PACK32;

		case TEXTURE_FORMAT::D16_UNORM:					return VK_FORMAT_D16_UNORM;
		case TEXTURE_FORMAT::D32_SFLOAT:				return VK_FORMAT_D32_SFLOAT;
		case TEXTURE_FORMAT::D24_UNORM_S8_UINT:			return VK_FORMAT_D24_UNORM_S8_UINT;

		case TEXTURE_FORMAT::BC1_RGB_UNORM:				return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
		case TEXTURE_FORMAT::BC1_RGB_SRGB:				return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
		case TEXTURE_FORMAT::BC3_RGBA_UNORM:			return VK_FORMAT_BC3_UNORM_BLOCK;
		case TEXTURE_FORMAT::BC3_RGBA_SRGB:				return VK_FORMAT_BC3_SRGB_BLOCK;
		case TEXTURE_FORMAT::BC4_R_UNORM:				return VK_FORMAT_BC4_UNORM_BLOCK;
		case TEXTURE_FORMAT::BC4_R_SNORM:				return VK_FORMAT_BC4_SNORM_BLOCK;
		case TEXTURE_FORMAT::BC5_RG_UNORM:				return VK_FORMAT_BC5_UNORM_BLOCK;
		case TEXTURE_FORMAT::BC5_RG_SNORM:				return VK_FORMAT_BC5_SNORM_BLOCK;
		case TEXTURE_FORMAT::BC7_RGBA_UNORM:			return VK_FORMAT_BC7_UNORM_BLOCK;
		case TEXTURE_FORMAT::BC7_RGBA_SRGB:				return VK_FORMAT_BC7_SRGB_BLOCK;

		case TEXTURE_FORMAT::R32_UINT:					return VK_FORMAT_R32_UINT;

		default:
		{
			throw std::runtime_error("GetVulkanFormat() default case reached. Format = " + std::to_string(std::to_underlying(_format)));
		}
		}
	}



	TEXTURE_FORMAT VulkanTexture::GetRHIFormat(VkFormat _format)
	{
		switch (_format)
		{
		case VK_FORMAT_UNDEFINED:					return TEXTURE_FORMAT::UNDEFINED;

		case VK_FORMAT_R8_UNORM:					return TEXTURE_FORMAT::R8_UNORM;
		case VK_FORMAT_R8G8_UNORM:					return TEXTURE_FORMAT::R8G8_UNORM;
		case VK_FORMAT_R8G8B8A8_UNORM:				return TEXTURE_FORMAT::R8G8B8A8_UNORM;
		case VK_FORMAT_R8G8B8A8_SRGB:				return TEXTURE_FORMAT::R8G8B8A8_SRGB;
		case VK_FORMAT_B8G8R8A8_UNORM:				return TEXTURE_FORMAT::B8G8R8A8_UNORM;
		case VK_FORMAT_B8G8R8A8_SRGB:				return TEXTURE_FORMAT::B8G8R8A8_SRGB;

		case VK_FORMAT_R16_SFLOAT:					return TEXTURE_FORMAT::R16_SFLOAT;
		case VK_FORMAT_R16G16_SFLOAT:				return TEXTURE_FORMAT::R16G16_SFLOAT;
		case VK_FORMAT_R16G16B16A16_SFLOAT:			return TEXTURE_FORMAT::R16G16B16A16_SFLOAT;

		case VK_FORMAT_R32_SFLOAT:					return TEXTURE_FORMAT::R32_SFLOAT;
		case VK_FORMAT_R32G32_SFLOAT:				return TEXTURE_FORMAT::R32G32_SFLOAT;
		case VK_FORMAT_R32G32B32A32_SFLOAT:			return TEXTURE_FORMAT::R32G32B32A32_SFLOAT;

		case VK_FORMAT_B10G11R11_UFLOAT_PACK32:		return TEXTURE_FORMAT::B10G11R11_UFLOAT_PACK32;
		case VK_FORMAT_A2R10G10B10_UNORM_PACK32:	return TEXTURE_FORMAT::R10G10B10A2_UNORM;

		case VK_FORMAT_D16_UNORM:					return TEXTURE_FORMAT::D16_UNORM;
		case VK_FORMAT_D32_SFLOAT:					return TEXTURE_FORMAT::D32_SFLOAT;
		case VK_FORMAT_D24_UNORM_S8_UINT:			return TEXTURE_FORMAT::D24_UNORM_S8_UINT;

		case VK_FORMAT_BC1_RGB_UNORM_BLOCK:			return TEXTURE_FORMAT::BC1_RGB_UNORM;
		case VK_FORMAT_BC1_RGB_SRGB_BLOCK:			return TEXTURE_FORMAT::BC1_RGB_SRGB;
		case VK_FORMAT_BC3_UNORM_BLOCK:				return TEXTURE_FORMAT::BC3_RGBA_UNORM;
		case VK_FORMAT_BC3_SRGB_BLOCK:				return TEXTURE_FORMAT::BC3_RGBA_SRGB;
		case VK_FORMAT_BC4_UNORM_BLOCK:				return TEXTURE_FORMAT::BC4_R_UNORM;
		case VK_FORMAT_BC4_SNORM_BLOCK:				return TEXTURE_FORMAT::BC4_R_SNORM;
		case VK_FORMAT_BC5_UNORM_BLOCK:				return TEXTURE_FORMAT::BC5_RG_UNORM;
		case VK_FORMAT_BC5_SNORM_BLOCK:				return TEXTURE_FORMAT::BC5_RG_SNORM;
		case VK_FORMAT_BC7_UNORM_BLOCK:				return TEXTURE_FORMAT::BC7_RGBA_UNORM;
		case VK_FORMAT_BC7_SRGB_BLOCK:				return TEXTURE_FORMAT::BC7_RGBA_SRGB;

		case VK_FORMAT_R32_UINT:					return TEXTURE_FORMAT::R32_UINT;

		default:
		{
			throw std::runtime_error("GetTextureFormat() default case reached. Format = " + std::to_string(std::to_underlying(_format)));
		}
		}
	}
	
}