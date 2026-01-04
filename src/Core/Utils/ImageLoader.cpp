#include "ImageLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#ifndef STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#endif
#include <stb_image_resize2.h> 

#include <stdexcept>
#include <cmath>
#include <vector>
#include <algorithm>
#ifdef max
	#undef max
#endif

namespace NK
{
	
	std::unordered_map<std::string, UniquePtr<ImageData>> ImageLoader::m_filepathToImageDataCache;


	ImageLoader::~ImageLoader()
	{
		ClearCache();
	}

	
	
	ImageData* ImageLoader::LoadImage(const std::string& _filepath, bool _flipImage, bool _srgb)
	{
		const std::unordered_map<std::string, UniquePtr<ImageData>>::iterator it{ m_filepathToImageDataCache.find(_filepath) };
		if (it != m_filepathToImageDataCache.end())
		{
			return it->second.get();
		}

		stbi_set_flip_vertically_on_load(_flipImage);
		
		int width, height, nrChannels;
		unsigned char* basePixels = stbi_load(_filepath.c_str(), &width, &height, &nrChannels, 4); //Force 4 channel STBI_rgb_alpha to align with GPU requirements and simplify resizing 
		if (!basePixels) 
		{ 
			throw std::runtime_error("ImageLoader::LoadImage() - Failed to load image at filepath: " + _filepath); 
		}

		ImageData imageData{};
		imageData.desc.size = glm::ivec3(width, height, 1);
		imageData.desc.dimension = TEXTURE_DIMENSION::DIM_2;
		imageData.desc.arrayTexture = false;
		imageData.desc.format = _srgb ? DATA_FORMAT::R8G8B8A8_SRGB : DATA_FORMAT::R8G8B8A8_UNORM;
		imageData.desc.usage = TEXTURE_USAGE_FLAGS::TRANSFER_DST_BIT;
		
		//Calculate mip levels
		const std::uint32_t numLevels{ static_cast<std::uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1 };
		imageData.desc.mipLevels = numLevels;

		//Calculate total size for buffer allocation
		std::size_t totalSize{ 0 };
		glm::ivec3 mipSize{ imageData.desc.size };
		for (std::uint32_t i{ 0 }; i < numLevels; ++i)
		{
			totalSize += mipSize.x * mipSize.y * 4; //4 bytes per pixel (RGBA8)
			mipSize.x = std::max(1, mipSize.x / 2);
			mipSize.y = std::max(1, mipSize.y / 2);
		}

		imageData.numBytes = totalSize;
		
		//Allocate a buffer for all mips
		imageData.data = static_cast<unsigned char*>(malloc(totalSize));

		
		//Copy all levels
		std::size_t currentOffset{ 0 };
		const std::size_t level0Size{ static_cast<std::size_t>(width * height * 4) };
		memcpy(imageData.data, basePixels, level0Size); //level 0
		stbi_image_free(basePixels); //made a copy so dont need the original anymore
		currentOffset += level0Size;
		std::int32_t prevW{ width };
		std::int32_t prevH{ height };

		for (std::uint32_t i{ 1 }; i < numLevels; ++i)
		{
			const std::int32_t currentW{ std::max(1, prevW / 2) };
			const std::int32_t currentH{ std::max(1, prevH / 2) };
			const std::size_t currentMipSize{ static_cast<std::size_t>(currentW * currentH * 4) };

			const unsigned char* src{ imageData.data + (currentOffset - (prevW * prevH * 4)) };
			unsigned char* dst{ imageData.data + currentOffset };

			//Generate the mip with stbir resize
			if (_srgb)
			{
				stbir_resize_uint8_srgb(src, prevW, prevH, 0, dst, currentW, currentH, 0, STBIR_RGBA);
			}
			else
			{
				stbir_resize_uint8_linear(src, prevW, prevH, 0, dst, currentW, currentH, 0, STBIR_RGBA);
			}

			//Prepare for next mip
			currentOffset += currentMipSize;
			prevW = currentW;
			prevH = currentH;
		}

		//Add to cache
		m_filepathToImageDataCache[_filepath] = UniquePtr<ImageData>(NK_NEW(ImageData, imageData));
		return m_filepathToImageDataCache[_filepath].get();
	}


	
	void ImageLoader::FreeImage(ImageData* _imageData)
	{
		if (!_imageData || !_imageData->data) { throw std::runtime_error("ImageLoader::FreeImage() - Attempted to free uninitialised / already freed image."); }

		for (std::unordered_map<std::string, UniquePtr<ImageData>>::iterator it{ m_filepathToImageDataCache.begin() }; it != m_filepathToImageDataCache.end(); ++it)
		{
			if (it->second.get() == _imageData)
			{
				//Use free instead of stbi_image_free because the memory was allocated with malloc
				free(_imageData->data); 
				m_filepathToImageDataCache.erase(it->first);
				return;
			}
		}
	}

	
	
	void ImageLoader::ClearCache()
	{
		for (std::unordered_map<std::string, UniquePtr<ImageData>>::iterator it{ m_filepathToImageDataCache.begin() }; it != m_filepathToImageDataCache.end(); ++it)
		{
			if (it->second->data)
			{
				free(it->second->data);
			}
		}
		m_filepathToImageDataCache.clear();
	}
	
}
