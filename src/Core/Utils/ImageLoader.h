#pragma once
#include <string>

#include "RHI/ITexture.h"
#ifdef LoadImage
	#undef LoadImage //Conflicts with ImageLoader::LoadImage()
#endif

namespace NK
{
	struct ImageData
	{
		unsigned char* data;
		std::size_t numBytes;
		TextureDesc desc;
	};

	
	class ImageLoader final
	{
	public:
		//_srgb = true if image is intended for viewing (e.g.: an albedo texture) - will result in srgb format
		//_srgb = false if image is intended for data-input (e.g.: a normal texture) - will result in unorm format
		[[nodiscard]] static ImageData LoadImage(const std::string& _filepath, bool _flipImage, bool _srgb);
		
		static void FreeImage(const ImageData& _imageData);
		
		
	private:
		//To avoid unnecessary duplicate loads
		static std::unordered_map<std::string, ImageData> filepathToImageDataCache;
	};
}