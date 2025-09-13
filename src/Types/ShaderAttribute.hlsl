//HLSL-only header

#include "ShaderAttributeLocations.h"

#define ATTRIBUTE(TYPE, NAME, LOCATION_ENUM, HLSL_SEMANTIC_STRING) \
		[[vk::location(LOCATION_ENUM)]] TYPE NAME : HLSL_SEMANTIC_STRING