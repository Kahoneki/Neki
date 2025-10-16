#pragma once

#include "Context.h"


namespace NK
{
	
	class RAIIContext final
	{
	public:
		explicit RAIIContext(const LoggerConfig& _loggerConfig, ALLOCATOR_TYPE _allocatorType)
		{
			Context::Initialise(_loggerConfig, _allocatorType);
		}



		~RAIIContext()
		{
			Context::Shutdown();
		}
	};
	
}