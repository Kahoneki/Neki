#pragma once
#include <string>
#include <Types/DataFormat.h>

namespace NK
{
	class RHIUtils
	{
	public:
		[[nodiscard]] static std::uint32_t GetFormatBytesPerPixel(DATA_FORMAT _format);
	};
}