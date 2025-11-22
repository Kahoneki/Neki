#pragma once

#include "TextureCompressor.h"

#include <thread>


namespace NK
{

	void TextureCompressor::BlockCompress(const std::string& _inputFilepath, const bool _srgb, const std::string& _outputFilepath, const DATA_FORMAT _outputFormat)
	{
		if (_outputFormat <= DATA_FORMAT::START_OF_BLOCK_COMPRESSION_FORMATS || _outputFormat >= DATA_FORMAT::END_OF_BLOCK_COMPRESSION_FORMATS)
		{
			throw std::invalid_argument("TextureCompressor::BlockCompress() - provided format (" + std::to_string(std::to_underlying(_outputFormat)) + ") is not a recognised block compression format as required");
		}
	}



	VkFormat TextureCompressor::GetVulkanFormat(const DATA_FORMAT _format)
	{
		//todo: it's pretty weird having this in both VulkanUtils and here....
		//todo^: for now though, i cant think of a better way of handling the whole redefining the vkformat enum for dx12 support
		switch (_format)
		{
		case DATA_FORMAT::UNDEFINED:				return VK_FORMAT_UNDEFINED;
			
		case DATA_FORMAT::R8_UNORM:					return VK_FORMAT_R8_UNORM;
		case DATA_FORMAT::R8G8_UNORM:				return VK_FORMAT_R8G8_UNORM;
		case DATA_FORMAT::R8G8B8A8_UNORM:			return VK_FORMAT_R8G8B8A8_UNORM;
		case DATA_FORMAT::R8G8B8A8_SRGB:			return VK_FORMAT_R8G8B8A8_SRGB;
		case DATA_FORMAT::B8G8R8A8_UNORM:			return VK_FORMAT_B8G8R8A8_UNORM;
		case DATA_FORMAT::B8G8R8A8_SRGB:			return VK_FORMAT_B8G8R8A8_SRGB;

		case DATA_FORMAT::R16_SFLOAT:				return VK_FORMAT_R16_SFLOAT;
		case DATA_FORMAT::R16G16_SFLOAT:			return VK_FORMAT_R16G16_SFLOAT;
		case DATA_FORMAT::R16G16B16A16_SFLOAT:		return VK_FORMAT_R16G16B16A16_SFLOAT;

		case DATA_FORMAT::R32_SFLOAT:				return VK_FORMAT_R32_SFLOAT;
		case DATA_FORMAT::R32G32_SFLOAT:			return VK_FORMAT_R32G32_SFLOAT;
		case DATA_FORMAT::R32G32B32_SFLOAT:			return VK_FORMAT_R32G32B32_SFLOAT;
		case DATA_FORMAT::R32G32B32A32_SFLOAT:		return VK_FORMAT_R32G32B32A32_SFLOAT;

		case DATA_FORMAT::B10G11R11_UFLOAT_PACK32:	return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
		case DATA_FORMAT::R10G10B10A2_UNORM:		return VK_FORMAT_A2R10G10B10_UNORM_PACK32;

		case DATA_FORMAT::D16_UNORM:				return VK_FORMAT_D16_UNORM;
		case DATA_FORMAT::D32_SFLOAT:				return VK_FORMAT_D32_SFLOAT;
		case DATA_FORMAT::D24_UNORM_S8_UINT:		return VK_FORMAT_D24_UNORM_S8_UINT;

		case DATA_FORMAT::BC1_RGB_UNORM:			return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
		case DATA_FORMAT::BC1_RGB_SRGB:				return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
		case DATA_FORMAT::BC3_RGBA_UNORM:			return VK_FORMAT_BC3_UNORM_BLOCK;
		case DATA_FORMAT::BC3_RGBA_SRGB:			return VK_FORMAT_BC3_SRGB_BLOCK;
		case DATA_FORMAT::BC4_R_UNORM:				return VK_FORMAT_BC4_UNORM_BLOCK;
		case DATA_FORMAT::BC4_R_SNORM:				return VK_FORMAT_BC4_SNORM_BLOCK;
		case DATA_FORMAT::BC5_RG_UNORM:				return VK_FORMAT_BC5_UNORM_BLOCK;
		case DATA_FORMAT::BC5_RG_SNORM:				return VK_FORMAT_BC5_SNORM_BLOCK;
		case DATA_FORMAT::BC7_RGBA_UNORM:			return VK_FORMAT_BC7_UNORM_BLOCK;
		case DATA_FORMAT::BC7_RGBA_SRGB:			return VK_FORMAT_BC7_SRGB_BLOCK;

		case DATA_FORMAT::R8_UINT:					return VK_FORMAT_R8_UINT;
		case DATA_FORMAT::R16_UINT:					return VK_FORMAT_R16_UINT;
		case DATA_FORMAT::R32_UINT:					return VK_FORMAT_R32_UINT;

		default:
		{
			throw std::invalid_argument("GetVulkanFormat() default case reached. Format = " + std::to_string(std::to_underlying(_format)));
		}
		}
	}

}