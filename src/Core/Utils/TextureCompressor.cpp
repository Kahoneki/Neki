#pragma once

#include "TextureCompressor.h"

#include <ktx.h>
#include <stb_image.h>
#include <thread>


namespace NK
{

	std::unordered_map<std::string, ImageData> TextureCompressor::m_filepathToImageDataCache;

	
	
	void TextureCompressor::KTXCompress(const std::string& _inputFilepath, const bool _srgb, const bool _flipImage, const std::string& _outputFilepath)
	{
		stbi_set_flip_vertically_on_load(_flipImage);
		
		int width, height, nrChannels;
		const int result{ stbi_info(_inputFilepath.c_str(), &width, &height, &nrChannels) };
		if (!result)
		{
			throw std::runtime_error("TextureCompressor::KTXCompress() - failed to read image info for " + _inputFilepath + " - result: " + std::to_string(result));
		}

		//Determine loading strategy
		//If rgb, promote to rgba to make the gpu happy
		//otherwise (1, 2, or 4) just keep it as is
		const int desiredChannels{ (nrChannels == 3) ? 4 : nrChannels };

		unsigned char* pixels{ stbi_load(_inputFilepath.c_str(), &width, &height, &nrChannels, desiredChannels) };
		if (!pixels)
		{
			throw std::runtime_error("TextureCompressor::KTXCompress() - failed to load image at " + _inputFilepath);
		}

		//determine input format
		VkFormat vkFormat{ VK_FORMAT_UNDEFINED };
		switch (desiredChannels)
		{
		case 1: vkFormat = _srgb ? VK_FORMAT_R8_SRGB : VK_FORMAT_R8_UNORM; break;
		case 2: vkFormat = _srgb ? VK_FORMAT_R8G8_SRGB : VK_FORMAT_R8G8_UNORM; break;
		case 4: vkFormat = _srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM; break;
		default:
			stbi_image_free(pixels);
			throw std::runtime_error("TextureCompressor::KTXCompress() - unsupported channel count: " + std::to_string(desiredChannels));
		}

		//create the ktx texture
		ktxTextureCreateInfo createInfo{};
		createInfo.vkFormat = vkFormat;
		createInfo.baseWidth = width;
		createInfo.baseHeight = height;
		createInfo.baseDepth = 1; //todo: 3d textures
		createInfo.numDimensions = 2; //todo: 3d textures
		createInfo.numLevels = 1; //todo: mips
		createInfo.numLayers = 1; //todo: array textures
		createInfo.numFaces = 1; //todo: cube textures
		createInfo.isArray = KTX_FALSE; //todo: array textures
		createInfo.generateMipmaps = KTX_FALSE; //todo: mips

		ktxTexture2* texture;
		KTX_error_code ktxResult{ ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &texture) };
		if (ktxResult != KTX_SUCCESS)
		{
			stbi_image_free(pixels);
			throw std::runtime_error("TextureCompressor::KTXCompress() - ktxTexture2_Create failed: " + std::string(ktxErrorString(ktxResult)));
		}

		ktxResult = ktxTexture_SetImageFromMemory(ktxTexture(texture), 0, 0, 0, pixels, width * height * desiredChannels); //todo: add array support, cube support, 3d texture support, mip support, yada yada - also why is this a macro
		stbi_image_free(pixels);
		if (ktxResult != KTX_SUCCESS)
		{
			ktxTexture2_Destroy(texture);
			throw std::runtime_error("TextureCompressor::KTXCompress() - ktxTexture_SetImageFromMemory failed: " + std::string(ktxErrorString(ktxResult)));
		}

		ktxBasisParams params{};
		params.structSize = sizeof(params);
		params.uastc = KTX_TRUE;
		params.uastcRDO = KTX_FALSE; //they're saying it's a free performance glitch? they're saying the devs forgot to patch this one simple trick? they're saying they're saying, they they....
		params.threadCount = std::thread::hardware_concurrency();
		ktxResult = ktxTexture2_CompressBasisEx(texture, &params);
		if (ktxResult != KTX_SUCCESS)
		{
			ktxTexture2_Destroy(texture);
			throw std::runtime_error("TextureCompressor::KTXCompress() - ktxTexture2_CompressBasisEx failed: " + std::string(ktxErrorString(ktxResult)));
		}

		//todo: try with and without this
//		//pre-transcode to bcn for fast loads
//		ktx_transcode_fmt_e targetFormat{ KTX_TTF_NOSELECTION };
//		switch (desiredChannels)
//		{
//		case 1:
//			targetFormat = KTX_TTF_BC4_R; 
//			break;
//		case 2: 
//			targetFormat = KTX_TTF_BC5_RG; 
//			break;
//		case 4: 
//			targetFormat = KTX_TTF_BC7_RGBA; 
//			break;
//		default:
//			throw std::runtime_error("TextureCompressor::BlockCompress() pre-transcode switch default case reached - desiredChannels = " + std::to_string(desiredChannels));
//		}
//
//		ktxResult = ktxTexture2_TranscodeBasis(texture, targetFormat, 0);
//		if (ktxResult != KTX_SUCCESS)
//		{
//			ktxTexture2_Destroy(texture);
//			throw std::runtime_error("TextureCompressor::BlockCompress() - ktxTexture2_TranscodeBasis failed: " + std::string(ktxErrorString(ktxResult)));
//		}

		ktxResult = ktxTexture_WriteToNamedFile(ktxTexture(texture), _outputFilepath.c_str());
		if (ktxResult != KTX_SUCCESS)
		{
			ktxTexture2_Destroy(texture);
			throw std::runtime_error("TextureCompressor::KTXCompress() - ktxTexture_WriteToNamedFile for filepath " + _outputFilepath + " failed: " + std::string(ktxErrorString(ktxResult)));
		}
	}



	ImageData* TextureCompressor::LoadImage(const std::string& _filepath, bool _flipImage, bool _srgb)
	{
		const std::unordered_map<std::string, ImageData>::iterator it{ m_filepathToImageDataCache.find(_filepath) };
		if (it != m_filepathToImageDataCache.end())
		{
			//Image has already been loaded, pull from cache
			return &(it->second);
		}
		
		ktxTexture2* texture;
		ktx_error_code_e ktxResult{ ktxTexture2_CreateFromNamedFile(_filepath.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture) };
		if (ktxResult != KTX_SUCCESS)
		{
			ktxTexture2_Destroy(texture);
			throw std::runtime_error("TextureCompressor::LoadImage() - ktxTexture2_CreateFromNamedFile for filepath " + _filepath + " failed: " + std::string(ktxErrorString(ktxResult)));
		}
		
		if (ktxTexture2_NeedsTranscoding(texture))
		{
			//Transcode to appropriate bcn
			
			ktx_transcode_fmt_e targetFormat{ KTX_TTF_NOSELECTION };

			//Select correct bcn format based on channel count
			const std::uint32_t numComponents{ ktxTexture2_GetNumComponents(texture) };
			if (numComponents == 1) { targetFormat = KTX_TTF_BC4_R; } //R8 -> BC4
			else if (numComponents == 2) { targetFormat = KTX_TTF_BC5_RG; } //RG8 -> BC5
			else { targetFormat = KTX_TTF_BC7_RGBA; } //RGB8 / RGBA8 -> BC7

			//Transcode from UASTC to BCn
			ktxResult = ktxTexture2_TranscodeBasis(texture, targetFormat, 0);
			if (ktxResult != KTX_SUCCESS)
			{
				ktxTexture2_Destroy(texture);
				throw std::runtime_error("TextureCompressor::LoadImage() - ktxTexture2_TranscodeBasis failed: " + std::string(ktxErrorString(ktxResult)));
			}
		}

		//Populate ImageData
		ImageData imageData{};
		imageData.desc.size.x = texture->baseWidth;
		imageData.desc.size.y = texture->baseHeight;
		imageData.desc.size.z = 1; //todo: 3d texture support, array support
		imageData.numBytes = texture->dataSize;
		imageData.desc.dimension = TEXTURE_DIMENSION::DIM_2; //todo: 3d texture support
		imageData.desc.arrayTexture = false; //todo: array support
		imageData.desc.format = GetRHIFormat(static_cast<VkFormat>(texture->vkFormat));
		imageData.desc.usage = TEXTURE_USAGE_FLAGS::TRANSFER_DST_BIT;
		imageData.data = texture->pData;

		//Add to cache
		m_filepathToImageDataCache[_filepath] = imageData;
		
		return &(m_filepathToImageDataCache[_filepath]);
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


	
	DATA_FORMAT TextureCompressor::GetRHIFormat(const VkFormat _format)
	{
		//todo: it's pretty weird having this in both VulkanUtils and here....
		//todo^: for now though, i cant think of a better way of handling the whole redefining the vkformat enum for dx12 support
		switch (_format)
		{
		case VK_FORMAT_UNDEFINED:					return DATA_FORMAT::UNDEFINED;

		case VK_FORMAT_R8_UNORM:					return DATA_FORMAT::R8_UNORM;
		case VK_FORMAT_R8G8_UNORM:					return DATA_FORMAT::R8G8_UNORM;
		case VK_FORMAT_R8G8B8A8_UNORM:				return DATA_FORMAT::R8G8B8A8_UNORM;
		case VK_FORMAT_R8G8B8A8_SRGB:				return DATA_FORMAT::R8G8B8A8_SRGB;
		case VK_FORMAT_B8G8R8A8_UNORM:				return DATA_FORMAT::B8G8R8A8_UNORM;
		case VK_FORMAT_B8G8R8A8_SRGB:				return DATA_FORMAT::B8G8R8A8_SRGB;

		case VK_FORMAT_R16_SFLOAT:					return DATA_FORMAT::R16_SFLOAT;
		case VK_FORMAT_R16G16_SFLOAT:				return DATA_FORMAT::R16G16_SFLOAT;
		case VK_FORMAT_R16G16B16A16_SFLOAT:			return DATA_FORMAT::R16G16B16A16_SFLOAT;

		case VK_FORMAT_R32_SFLOAT:					return DATA_FORMAT::R32_SFLOAT;
		case VK_FORMAT_R32G32_SFLOAT:				return DATA_FORMAT::R32G32_SFLOAT;
		case VK_FORMAT_R32G32B32A32_SFLOAT:			return DATA_FORMAT::R32G32B32A32_SFLOAT;

		case VK_FORMAT_B10G11R11_UFLOAT_PACK32:		return DATA_FORMAT::B10G11R11_UFLOAT_PACK32;
		case VK_FORMAT_A2R10G10B10_UNORM_PACK32:	return DATA_FORMAT::R10G10B10A2_UNORM;

		case VK_FORMAT_D16_UNORM:					return DATA_FORMAT::D16_UNORM;
		case VK_FORMAT_D32_SFLOAT:					return DATA_FORMAT::D32_SFLOAT;
		case VK_FORMAT_D24_UNORM_S8_UINT:			return DATA_FORMAT::D24_UNORM_S8_UINT;

		case VK_FORMAT_BC1_RGB_UNORM_BLOCK:			return DATA_FORMAT::BC1_RGB_UNORM;
		case VK_FORMAT_BC1_RGB_SRGB_BLOCK:			return DATA_FORMAT::BC1_RGB_SRGB;
		case VK_FORMAT_BC3_UNORM_BLOCK:				return DATA_FORMAT::BC3_RGBA_UNORM;
		case VK_FORMAT_BC3_SRGB_BLOCK:				return DATA_FORMAT::BC3_RGBA_SRGB;
		case VK_FORMAT_BC4_UNORM_BLOCK:				return DATA_FORMAT::BC4_R_UNORM;
		case VK_FORMAT_BC4_SNORM_BLOCK:				return DATA_FORMAT::BC4_R_SNORM;
		case VK_FORMAT_BC5_UNORM_BLOCK:				return DATA_FORMAT::BC5_RG_UNORM;
		case VK_FORMAT_BC5_SNORM_BLOCK:				return DATA_FORMAT::BC5_RG_SNORM;
		case VK_FORMAT_BC7_UNORM_BLOCK:				return DATA_FORMAT::BC7_RGBA_UNORM;
		case VK_FORMAT_BC7_SRGB_BLOCK:				return DATA_FORMAT::BC7_RGBA_SRGB;

		case VK_FORMAT_R32_UINT:					return DATA_FORMAT::R32_UINT;

		default:
		{
			throw std::invalid_argument("GetRHIFormat() default case reached. Format = " + std::to_string(_format));
		}
		}
	}
}