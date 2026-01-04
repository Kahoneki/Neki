#include "TextureCompressor.h"

#include <RHI/RHIUtils.h>

#include <cmath>
#include <ktx.h>
#include <stb_image.h>
#include <stb_image_resize2.h>
#include <thread>


namespace NK
{

	std::unordered_map<std::string, UniquePtr<ImageData>> TextureCompressor::m_filepathToImageDataCache;


	TextureCompressor::~TextureCompressor()
	{
		ClearCache();
	}

	
	
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

		//Determine num mip levels (down to 1x1)
		const std::uint32_t numLevels{ static_cast<std::uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1 };
		
		//Determine input format and resize layout
		VkFormat vkFormat{ VK_FORMAT_UNDEFINED };
		stbir_pixel_layout resizeLayout;
		switch (desiredChannels)
		{
		case 1: 
			vkFormat = _srgb ? VK_FORMAT_R8_SRGB : VK_FORMAT_R8_UNORM; 
			resizeLayout = STBIR_1CHANNEL;
			break;
		case 2: 
			vkFormat = _srgb ? VK_FORMAT_R8G8_SRGB : VK_FORMAT_R8G8_UNORM; 
			resizeLayout = STBIR_RA;
			break;
		case 4: 
			vkFormat = _srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM; 
			resizeLayout = STBIR_RGBA;
			break;
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
		createInfo.numLayers = 1; //todo: array textures
		createInfo.numFaces = 1; //todo: cube textures
		createInfo.isArray = KTX_FALSE; //todo: array textures
		createInfo.generateMipmaps = KTX_FALSE;
		createInfo.numLevels = numLevels;

		ktxTexture2* texture;
		KTX_error_code ktxResult{ ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &texture) };
		if (ktxResult != KTX_SUCCESS)
		{
			stbi_image_free(pixels);
			throw std::runtime_error("TextureCompressor::KTXCompress() - ktxTexture2_Create failed: " + std::string(ktxErrorString(ktxResult)));
		}

		//Set level 0 (base image)
		ktxResult = ktxTexture_SetImageFromMemory(ktxTexture(texture), 0, 0, 0, pixels, width * height * desiredChannels); //todo: add array support, cube support, 3d texture support, yada yada - also why is this a macro
		if (ktxResult != KTX_SUCCESS)
		{
			stbi_image_free(pixels);
			ktxTexture2_Destroy(texture);
			throw std::runtime_error("TextureCompressor::KTXCompress() - ktxTexture_SetImageFromMemory (level 0) failed: " + std::string(ktxErrorString(ktxResult)));
		}

		//Generate and set mipmaps (levels 1 to numLevels)
		std::vector<unsigned char> srcBuffer(pixels, pixels + (width * height * desiredChannels));
		stbi_image_free(pixels); //Free original STBI memory (it's been stored in the vector)

		std::int32_t currentW{ width };
		std::int32_t currentH{ height };

		//Loop through all levels and generate the mip
		for (int level = 1; level < numLevels; ++level)
		{
			const std::int32_t nextW{ std::max(1, currentW / 2) };
			const std::int32_t nextH{ std::max(1, currentH / 2) };
			const std::int32_t nextSize{ nextW * nextH * desiredChannels };
			std::vector<unsigned char> dstBuffer(nextSize);

			//Generate the mip with stbir resize
			if (_srgb)
			{
				stbir_resize_uint8_srgb(srcBuffer.data(), currentW, currentH, 0, dstBuffer.data(), nextW, nextH, 0, resizeLayout);
			}
			else
			{
				stbir_resize_uint8_linear(srcBuffer.data(), currentW, currentH, 0, dstBuffer.data(), nextW, nextH, 0, resizeLayout);
			}

			//Set in ktx texture
			ktxResult = ktxTexture_SetImageFromMemory(ktxTexture(texture), level, 0, 0, dstBuffer.data(), dstBuffer.size());
			if (ktxResult != KTX_SUCCESS)
			{
				ktxTexture2_Destroy(texture);
				throw std::runtime_error("TextureCompressor::KTXCompress() - SetImageFromMemory (Level " + std::to_string(level) + ") failed: " + std::string(ktxErrorString(ktxResult)));
			}

			//Prepare for next mip
			srcBuffer = std::move(dstBuffer);
			currentW = nextW;
			currentH = nextH;
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
		const std::unordered_map<std::string, UniquePtr<ImageData>>::iterator it{ m_filepathToImageDataCache.find(_filepath) };
		if (it != m_filepathToImageDataCache.end())
		{
			//Image has already been loaded, pull from cache
			return it->second.get();
		}
		
		ktxTexture* texture;
		ktx_error_code_e ktxResult{ ktxTexture_CreateFromNamedFile(_filepath.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture) };
		if (ktxResult != KTX_SUCCESS)
		{
			throw std::runtime_error("TextureCompressor::LoadImage() - ktxTexture2_CreateFromNamedFile for filepath " + _filepath + " failed: " + std::string(ktxErrorString(ktxResult)));
		}
		
		if (texture->classId == ktxTexture2_c)
		{
			ktxTexture2* texture2{ reinterpret_cast<ktxTexture2*>(texture) };
			if (ktxTexture2_NeedsTranscoding(texture2))
			{
				//Transcode to appropriate bcn
				ktx_transcode_fmt_e targetFormat{ KTX_TTF_NOSELECTION };

				//Select correct bcn format based on channel count
				const std::uint32_t numComponents{ ktxTexture2_GetNumComponents(texture2) };
				if (numComponents == 1) { targetFormat = KTX_TTF_BC4_R; } //R8 -> BC4
				else if (numComponents == 2) { targetFormat = KTX_TTF_BC5_RG; } //RG8 -> BC5
				else { targetFormat = KTX_TTF_BC7_RGBA; } //RGB8 / RGBA8 -> BC7

				//Transcode from UASTC to BCn
				ktxResult = ktxTexture2_TranscodeBasis(texture2, targetFormat, 0);
				if (ktxResult != KTX_SUCCESS)
				{
					ktxTexture2_Destroy(texture2);
					throw std::runtime_error("TextureCompressor::LoadImage() - ktxTexture2_TranscodeBasis failed: " + std::string(ktxErrorString(ktxResult)));
				}
			}
		}

		//Populate ImageData
		ImageData imageData{};
		imageData.desc.size.x = texture->baseWidth;
		imageData.desc.size.y = texture->baseHeight;
		imageData.desc.mipLevels = texture->numLevels;
		imageData.desc.dimension = TEXTURE_DIMENSION::DIM_2;
		imageData.desc.usage = TEXTURE_USAGE_FLAGS::TRANSFER_DST_BIT;
		
		//Handle cube/array dimensions
		if (texture->isCubemap)
		{
			imageData.desc.cubemap = true;
			imageData.desc.arrayTexture = true;
			imageData.desc.size.z = 6;
		}
		else if (texture->numLayers > 1)
		{
			imageData.desc.cubemap = false;
			imageData.desc.arrayTexture = true;
			imageData.desc.size.z = texture->numLayers;
		}
		else
		{
			imageData.desc.cubemap = false;
			imageData.desc.arrayTexture = false;
			imageData.desc.size.z = 1;
		}

		//Get Format
		//^todo: there has to be a better way of doing this.... oh almighty khronos group why must you force vulkan upon us
		if (texture->classId == ktxTexture2_c)
		{
			ktxTexture2* texture2{ reinterpret_cast<ktxTexture2*>(texture) };
			imageData.desc.format = GetRHIFormat(static_cast<VkFormat>(texture2->vkFormat));
		}
		else
		{
			const ktxTexture1* t1{ reinterpret_cast<ktxTexture1*>(texture) };
			if (t1->glInternalformat == 0x881A) //opengl bytecode for the rgba16sf format
			{
				imageData.desc.format = DATA_FORMAT::R16G16B16A16_SFLOAT;
			}
			else if (t1->glInternalformat == 0x8058) //opengl bytecode for the rgba8 format
			{
				imageData.desc.format = _srgb ? DATA_FORMAT::R8G8B8A8_SRGB : DATA_FORMAT::R8G8B8A8_UNORM;
			}
			else //good luck lmao
			{
				throw std::runtime_error("TextureCompressor::LoadImage() - Unsupported KTX1 OGL Format: " + std::to_string(t1->glInternalformat));
			}
		}

		//Calculate total (tightly-packed) size and all mip offsets
		std::size_t totalTightSize{ 0 };
		std::vector<std::size_t> mipOffsets(texture->numLevels);
		
		glm::uvec3 mipSize{ imageData.desc.size };
		for(std::uint32_t mip{ 0 }; mip < texture->numLevels; ++mip)
		{
			mipOffsets[mip] = totalTightSize;

			//Calculate size of this mip
			std::size_t mipBytes{ 0 };
			if (RHIUtils::IsBlockCompressed(imageData.desc.format))
			{
				const uint32_t blocksX{ (mipSize.x + 3) / 4 };
				const uint32_t blocksY{ (mipSize.y + 3) / 4 };
				mipBytes = blocksX * blocksY * RHIUtils::GetBlockByteSize(imageData.desc.format);
			}
			else
			{
				mipBytes = mipSize.x * mipSize.y * RHIUtils::GetFormatBytesPerPixel(imageData.desc.format);
			}
			
			//Multiply by array layers
			mipBytes *= imageData.desc.size.z;

			//Prepare for next mip
			totalTightSize += mipBytes;
			mipSize.x = std::max(1u, mipSize.x / 2);
			mipSize.y = std::max(1u, mipSize.y / 2);
		}

		imageData.numBytes = totalTightSize;
		
		//Allocate a flat buffer to copy
		//(use malloc so stbi_image_free (which wraps free) in FreeImage can clean it up safely)
		imageData.data = static_cast<unsigned char*>(malloc(totalTightSize));
		const std::uint32_t layers{ static_cast<std::uint32_t>(imageData.desc.size.z) };
		mipSize = imageData.desc.size;

		//Copy each mip
		for(std::uint32_t mip{ 0 }; mip < texture->numLevels; ++mip)
		{
			std::size_t layerBytes{ 0 };
			if (RHIUtils::IsBlockCompressed(imageData.desc.format))
			{
				layerBytes = ((mipSize.x + 3) / 4) * ((mipSize.y + 3) / 4) * RHIUtils::GetBlockByteSize(imageData.desc.format);
			}
			else
			{
				layerBytes = mipSize.x * mipSize.y * RHIUtils::GetFormatBytesPerPixel(imageData.desc.format);
			}

			//Copy each layer for the current mip
			for(uint32_t layer{ 0 }; layer < layers; ++layer)
			{
				//Get offset for current mip+layer from KTX (it sometimes does some internal padding)
				ktx_size_t srcOffset;
				ktxTexture_GetImageOffset(texture, mip, 0, layer, &srcOffset);
				const std::size_t dstOffset{ mipOffsets[mip] + (layer * layerBytes) };
				memcpy(imageData.data + dstOffset, texture->pData + srcOffset, layerBytes);
			}

			//Prepare for next mip
			mipSize.x = std::max(1u, mipSize.x / 2);
			mipSize.y = std::max(1u, mipSize.y / 2);
		}
		
		ktxTexture_Destroy(texture);

		//Add to cache
		m_filepathToImageDataCache[_filepath] = UniquePtr<ImageData>(NK_NEW(ImageData, imageData));
		return m_filepathToImageDataCache[_filepath].get();
	}
	
	
	
	void TextureCompressor::FreeImage(const ImageData* _imageData)
	{
		if (!_imageData || !_imageData->data) { return; }
		for (std::unordered_map<std::string, UniquePtr<ImageData>>::iterator it{ m_filepathToImageDataCache.begin() }; it != m_filepathToImageDataCache.end(); ++it)
		{
			if (it->second.get() == _imageData)
			{
				free(it->second->data);
				it->second->data = nullptr;
				m_filepathToImageDataCache.erase(it);
				return;
			}
		}
	}

	
	
	void TextureCompressor::ClearCache()
	{
		for (std::unordered_map<std::string, UniquePtr<ImageData>>::iterator it{ m_filepathToImageDataCache.begin() }; it != m_filepathToImageDataCache.end(); ++it)
		{
			if (it->second->data)
			{
				free(it->second->data);
				it->second->data = nullptr;
			}
		}
		m_filepathToImageDataCache.clear();
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