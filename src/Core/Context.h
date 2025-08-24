#pragma once
#include "Debug/ILogger.h"
#include "Memory/IAllocator.h"

namespace NK
{
	//Global static context class
	class Context
	{
	public:
		inline static ILogger* GetLogger() { return m_logger; }
		inline static IAllocator* GetAllocator() { return m_allocator; }


	private:
		Context(const LoggerConfig& _loggerConfig, ALLOCATOR_TYPE _allocatorType);
		~Context();
		static ILogger* m_logger;
		static IAllocator* m_allocator;
	};
}