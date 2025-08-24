#include "Context.h"

#include <stdexcept>

#include "Debug/ConsoleLogger.h"
#include "Memory/TrackingAllocator.h"

namespace NK
{
	ILogger* Context::m_logger{ nullptr };
	IAllocator* Context::m_allocator{ nullptr };



	Context::Context(const LoggerConfig& _loggerConfig, ALLOCATOR_TYPE _allocatorType)
	{
		switch (_loggerConfig.type)
		{
		case LOGGER_TYPE::CONSOLE: m_logger = new ConsoleLogger(_loggerConfig);
			break;
		default: throw std::runtime_error("Context::Context() - _loggerConfig.type not recognised.\n");
		}

		switch (_allocatorType)
		{
		case ALLOCATOR_TYPE::TRACKING: m_allocator = new TrackingAllocator(m_logger, false);
			break;
		case ALLOCATOR_TYPE::TRACKING_VERBOSE: m_allocator = new TrackingAllocator(m_logger, true);
			break;
		}

		m_logger->Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::CONTEXT, "Context Initialised\n");
	}



	Context::~Context()
	{
		m_logger->Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::CONTEXT, "Shutting Down Context\n");

		delete m_logger;
		delete m_allocator;
	}

}