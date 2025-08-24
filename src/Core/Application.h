#pragma once
#include "Debug/ILogger.h"
#include "Memory/IAllocator.h"

namespace NK
{
	class Application
	{
		Application();
		~Application();
		ILogger* m_logger;
		IAllocator* m_allocator;
	};
}