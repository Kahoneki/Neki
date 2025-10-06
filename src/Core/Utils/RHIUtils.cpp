#include "RHIUtils.h"
#include <stdexcept>

namespace NK
{

	std::uint32_t RHIUtils::GetFormatBytesPerPixel(DATA_FORMAT _format)
	{
		switch (_format)
		{
		case DATA_FORMAT::R8_UNORM:
		case DATA_FORMAT::R8_UINT:
			return 1;

		case DATA_FORMAT::R8G8_UNORM:
			return 2;

		case DATA_FORMAT::R8G8B8A8_UNORM:
		case DATA_FORMAT::R8G8B8A8_SRGB:
		case DATA_FORMAT::B8G8R8A8_UNORM:
		case DATA_FORMAT::B8G8R8A8_SRGB:
			return 4;

		case DATA_FORMAT::R16_SFLOAT:
		case DATA_FORMAT::R16_UINT:
			return 2;

		case DATA_FORMAT::R16G16_SFLOAT:
			return 4;

		case DATA_FORMAT::R16G16B16A16_SFLOAT:
			return 8;

		case DATA_FORMAT::R32_SFLOAT:
		case DATA_FORMAT::R32_UINT:
			return 4;

		case DATA_FORMAT::R32G32_SFLOAT:
			return 8;

		case DATA_FORMAT::R32G32B32_SFLOAT:
			return 12;

		case DATA_FORMAT::R32G32B32A32_SFLOAT:
			return 16;

		case DATA_FORMAT::B10G11R11_UFLOAT_PACK32:
		case DATA_FORMAT::R10G10B10A2_UNORM:
			return 4;

		case DATA_FORMAT::D16_UNORM:
			return 2;

		case DATA_FORMAT::D32_SFLOAT:
			return 4;

		case DATA_FORMAT::D24_UNORM_S8_UINT:
			return 4;

		//Note: For block-compressed formats, this returns bytes per block, not per pixel.
		case DATA_FORMAT::BC1_RGB_UNORM:
		case DATA_FORMAT::BC1_RGB_SRGB:
		case DATA_FORMAT::BC4_R_UNORM:
		case DATA_FORMAT::BC4_R_SNORM:
			return 8;

		case DATA_FORMAT::BC3_RGBA_UNORM:
		case DATA_FORMAT::BC3_RGBA_SRGB:
		case DATA_FORMAT::BC5_RG_UNORM:
		case DATA_FORMAT::BC5_RG_SNORM:
		case DATA_FORMAT::BC7_RGBA_UNORM:
		case DATA_FORMAT::BC7_RGBA_SRGB:
			return 16;

		default:
		{
			throw std::runtime_error("GetFormatBytesPerPixel() default case reached. Format = " + std::to_string(std::to_underlying(_format)));
		}
		}
	}

}