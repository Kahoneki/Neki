#pragma once

#include "Context.h"


namespace NK
{
	
	class RAIIContext final
	{
	public:
		explicit RAIIContext(const ContextConfig& _config)
		{
			Context::Initialise(_config);
		}



		~RAIIContext()
		{
			Context::Shutdown();
		}
	};
	
}