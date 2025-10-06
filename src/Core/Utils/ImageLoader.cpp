#include "ImageLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <stdexcept>


namespace NK
{
	std::unordered_map<std::string, ImageData> ImageLoader::filepathToImageDataCache;



	ImageData ImageLoader::LoadImage(const std::string& _filepath, bool _flipImage, bool _srgb)
	{
		const std::unordered_map<std::string, ImageData>::iterator it{ filepathToImageDataCache.find(_filepath) };
		if (it != filepathToImageDataCache.end())
		{
			//Image has already been loaded, pull from cache
			return it->second;
		}

		//Load image data
		stbi_set_flip_vertically_on_load(_flipImage);
		ImageData imageData{};
		int width, height, nrChannels;
		imageData.data = stbi_load(_filepath.c_str(), &width, &height, &nrChannels, 4); //Force 4-channel STBI_rgb_alpha
		if (!imageData.data) { throw std::runtime_error("ImageLoader::LoadImage() - Failed to load image at filepath: " + _filepath); }
		imageData.numBytes = width * height * nrChannels;
		imageData.desc.size = glm::ivec3(width, height, 1);
		imageData.desc.dimension = TEXTURE_DIMENSION::DIM_2;
		imageData.desc.arrayTexture = false;
		imageData.desc.format = _srgb ? DATA_FORMAT::R8G8B8A8_SRGB : DATA_FORMAT::R8G8B8A8_UNORM;
		imageData.desc.usage = TEXTURE_USAGE_FLAGS::TRANSFER_DST_BIT;

		//Add to cache
		filepathToImageDataCache[_filepath] = imageData;
		
		return imageData;
	}



	void ImageLoader::FreeImage(const ImageData& _imageData)
	{
		if (!_imageData.data) { throw std::runtime_error("ImageLoader::FreeImage() - Attempted to free uninitialised / already freed image."); }

		//Remove from cache
		for (const std::pair<std::string, ImageData> imgCacheEntry : filepathToImageDataCache)
		{
			if (imgCacheEntry.second.data == _imageData.data)
			{
				filepathToImageDataCache.erase(imgCacheEntry.first);
				break;
			}
		}

		stbi_image_free(_imageData.data);
	}

}