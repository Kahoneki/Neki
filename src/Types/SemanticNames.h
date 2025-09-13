#pragma once

namespace NK
{
	//HLSL doesn't recognise enum classes
	#ifdef HLSL
	enum SHADER_ATTRIBUTE
	#else
	enum class SHADER_ATTRIBUTE
	#endif
	{
		POSITION	= 0,
		NORMAL		= 1,
		TEXCOORD_0	= 2,
	};
}

#define SEMANTIC_NAME_POSITION POSITION
#define SEMANTIC_NAME_NORMAL NORMAL
#define SEMANTIC_NAME_TEXCOORD0 TEXCOORD0