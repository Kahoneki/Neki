#pragma once

#include "Debug/LoggerConfig.h"


namespace NK
{
	
	struct ContextConfig
	{
		LoggerConfig loggerConfig;
		AllocatorConfig allocatorDesc;
	};
	
}