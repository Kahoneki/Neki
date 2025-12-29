#pragma once

#include "Debug/LoggerConfig.h"


namespace NK
{
	
	struct ContextConfig
	{
		LoggerConfig loggerConfig;
		AllocatorConfig allocatorDesc;
		float fixedUpdateTimestep{ 1.0f / 60.0f }; //In seconds (Default: 1.0f / 60.0f)
	};
	
}