#pragma once

#include "Debug/ILogger.h"
#include "Memory/IAllocator.h"


namespace NK
{
	
	//Global static context class
	class Context
	{
	public:
		static void Initialise(const LoggerConfig& _loggerConfig, ALLOCATOR_TYPE _allocatorType);
		static void Shutdown();
		
		Context() = delete;
		~Context() = delete;

		inline static ILogger* GetLogger() { return m_logger; }
		inline static IAllocator* GetAllocator() { return m_allocator; }


	protected:
		static ILogger* m_logger;
		static IAllocator* m_allocator;
	};
	
}