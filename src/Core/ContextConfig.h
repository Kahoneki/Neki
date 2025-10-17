#pragma once

#include "Debug/LoggerConfig.h"


namespace NK
{
	
	struct ContextConfig
	{
		LoggerConfig loggerConfig;
		ALLOCATOR_TYPE allocatorType;
	};
	
}