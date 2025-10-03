//HLSL-only header
#ifndef SHADER_ATTRIBUTE_HLSLI
#define SHADER_ATTRIBUTE_HLSLI

#include "ShaderAttributeLocations.h"



//VERTEX INPUT ATTRIBUTES
#define ATTRIBUTE(TYPE, NAME, LOCATION_ENUM, HLSL_SEMANTIC_STRING) \
        [[vk::location(LOCATION_ENUM)]] TYPE NAME : HLSL_SEMANTIC_STRING



//PUSH CONSTANTS
//Example usage:
//PUSH_CONSTANTS_BLOCK(
//	float4x4 modelMatrix;
//	uint textureIndex;
//);
//The above can then be accessed using PC(modelMatrix) for example

#define PC_DECL_NAME PushConstants
#define PC_DEF_NAME pushConstants


#if defined(__spirv__)
	#define PUSH_CONSTANTS_BLOCK(DATA) \
	struct PC_DECL_NAME { DATA }; \
	[[vk::push_constant]] PC_DECL_NAME PC_DEF_NAME

	#define PC(NAME) PC_DEF_NAME.NAME
 

#else //dxil
	#define PUSH_CONSTANTS_BLOCK(DATA) \
	cbuffer PC_DECL_NAME : register(b0, space0) { DATA }

	#define PC(NAME) NAME


#endif



#endif
