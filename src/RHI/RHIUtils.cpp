#include "RHIUtils.h"
#include <stdexcept>
#include <utility>

namespace NK
{

	std::uint32_t RHIUtils::GetFormatBytesPerPixel(const DATA_FORMAT _format)
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
			throw std::invalid_argument("GetFormatBytesPerPixel() default case reached. Format = " + std::to_string(std::to_underlying(_format)));
		}
		}
	}



	bool RHIUtils::IsBlockCompressed(const DATA_FORMAT _format)
	{
		return (_format > DATA_FORMAT::START_OF_BLOCK_COMPRESSION_FORMATS) && (_format < DATA_FORMAT::END_OF_BLOCK_COMPRESSION_FORMATS);
	}


	
	std::uint32_t RHIUtils::GetBlockByteSize(const DATA_FORMAT _format)
	{
		switch (_format)
		{
		//8 Bytes per block (64 bits)
		case DATA_FORMAT::BC1_RGB_UNORM:
		case DATA_FORMAT::BC1_RGB_SRGB:
		case DATA_FORMAT::BC4_R_UNORM:
		case DATA_FORMAT::BC4_R_SNORM:
			return 8;

		//16 Bytes per block (128 bits)
		case DATA_FORMAT::BC3_RGBA_UNORM:
		case DATA_FORMAT::BC3_RGBA_SRGB:
		case DATA_FORMAT::BC5_RG_UNORM:
		case DATA_FORMAT::BC5_RG_SNORM:
		case DATA_FORMAT::BC7_RGBA_UNORM:
		case DATA_FORMAT::BC7_RGBA_SRGB:
			return 16;

		default:
			//Not a BCn format
			throw std::invalid_argument("GetBlockByteSize() default case reached. Format = " + std::to_string(std::to_underlying(_format)));
		}
	}



	std::uint32_t RHIUtils::Convert8BitMaskTo32BitMask(const std::uint8_t _mask)
	{
		//For each set bit in the input mask, the corresponding 4 bits in the output mask will be set
		std::uint32_t result{ 0 };
		for (int i{ 0 }; i < 8; ++i)
		{
			if ((_mask >> i) & 1)
			{
				result |= (0b1111 << (i * 4));
			}
		}
		return result;
	}



	std::string RHIUtils::GetCommandTypeString(const COMMAND_TYPE _type)
	{
		switch (_type)
		{
		case COMMAND_TYPE::GRAPHICS:	return "GRAPHICS";
		case COMMAND_TYPE::COMPUTE:		return "COMPUTE";
		case COMMAND_TYPE::TRANSFER:	return "TRANSFER";
		default:
		{
			throw std::invalid_argument("GetCommandTypeString() - switch case returned default. type = " + std::to_string(std::to_underlying(_type)));
		}
		}
	}



	RESOURCE_ACCESS_TYPE RHIUtils::GetResourceAccessType(const RESOURCE_STATE _state)
	{
		switch (_state)
		{
		case RESOURCE_STATE::UNDEFINED:
		{
			throw std::invalid_argument("RHIUtils::GetResourceAccessType() - _state (" + std::to_string(std::to_underlying(_state)) + ") - state has no access type");
		}
		case RESOURCE_STATE::PRESENT:
		case RESOURCE_STATE::VERTEX_BUFFER:
		case RESOURCE_STATE::INDEX_BUFFER:
		case RESOURCE_STATE::CONSTANT_BUFFER:
		case RESOURCE_STATE::INDIRECT_BUFFER:
		case RESOURCE_STATE::SHADER_RESOURCE:
		case RESOURCE_STATE::COPY_SOURCE:
		case RESOURCE_STATE::DEPTH_READ:
		case RESOURCE_STATE::RESOLVE_SOURCE:
			return RESOURCE_ACCESS_TYPE::READ;
			
		case RESOURCE_STATE::RENDER_TARGET:
		case RESOURCE_STATE::UNORDERED_ACCESS:
		case RESOURCE_STATE::COPY_DEST:
		case RESOURCE_STATE::DEPTH_WRITE:
		case RESOURCE_STATE::RESOLVE_DEST:
			return RESOURCE_ACCESS_TYPE::READ_WRITE;
		}
	}

}