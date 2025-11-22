#pragma once

#include <Types/NekiTypes.h>

#include <string>


namespace NK
{
	
	class RHIUtils
	{
	public:
		[[nodiscard]] static std::uint32_t GetFormatBytesPerPixel(const DATA_FORMAT _format);
		[[nodiscard]] static bool IsBlockCompressed(const DATA_FORMAT _format);
		[[nodiscard]] static std::uint32_t GetBlockByteSize(const DATA_FORMAT _format);
		[[nodiscard]] static std::uint32_t Convert8BitMaskTo32BitMask(const std::uint8_t _mask);
		[[nodiscard]] static std::string GetCommandTypeString(const COMMAND_TYPE _type);
		[[nodiscard]] static RESOURCE_ACCESS_TYPE GetResourceAccessType(const RESOURCE_STATE _state);
	};
	
}