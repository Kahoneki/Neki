#pragma once
#include <cstdint>

namespace NK
{
	enum class DATA_FORMAT : std::uint32_t
	{
		UNDEFINED = 0,

		//8-bit Colour Formats
		R8_UNORM,
		R8G8_UNORM,
		R8G8B8A8_UNORM,
		R8G8B8A8_SRGB,
		B8G8R8A8_UNORM,
		B8G8R8A8_SRGB,

		//16-bit Colour Formats
		R16_SFLOAT,
		R16G16_SFLOAT,
		R16G16B16A16_SFLOAT,

		//32-bit Colour Formats
		R32_SFLOAT,
		R32G32_SFLOAT,
		R32G32B32A32_SFLOAT,

		//Packed Formats
		B10G11R11_UFLOAT_PACK32,
		R10G10B10A2_UNORM,

		//Depth/Stencil Formats
		D16_UNORM,
		D32_SFLOAT,
		D24_UNORM_S8_UINT,

		//Block Compression / DXT Formats
		BC1_RGB_UNORM,
		BC1_RGB_SRGB,
		BC3_RGBA_UNORM,
		BC3_RGBA_SRGB,
		BC4_R_UNORM,
		BC4_R_SNORM,
		BC5_RG_UNORM,
		BC5_RG_SNORM,
		BC7_RGBA_UNORM,
		BC7_RGBA_SRGB,

		//Integer Formats
		R32_UINT,
	};	
}