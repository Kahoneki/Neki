//CPP-only header
#pragma once
#include <cstdint>
#include "ShaderAttributeLocations.h"

namespace NK
{
	enum class SHADER_ATTRIBUTE : std::uint32_t
	{
		POSITION = SHADER_ATTRIBUTE_LOCATION_POSITION,
		NORMAL = SHADER_ATTRIBUTE_LOCATION_NORMAL,
		TEXCOORD_0 = SHADER_ATTRIBUTE_LOCATION_TEXCOORD_0,
	};
}