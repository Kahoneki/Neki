#include "Context.h"

#include "Debug/ConsoleLogger.h"
#include "Memory/TrackingAllocator.h"

#include <stdexcept>
#include <GLFW/glfw3.h>


namespace NK
{
	
	ILogger* Context::m_logger{ nullptr };
	IAllocator* Context::m_allocator{ nullptr };


	void GLFWErrorCallback(int _error, const char* _description)
	{
		Context::GetLogger()->IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::GLFW, _description);
	}



	void Context::Initialise(const ContextConfig& _config)
	{
		switch (_config.loggerConfig.type)
		{
		case LOGGER_TYPE::CONSOLE: m_logger = new ConsoleLogger(_config.loggerConfig);
			break;
		default: throw std::runtime_error("Context::Context() - _config.loggerConfig.type not recognised.\n");
		}

		switch (_config.allocatorDesc.type)
		{
		case ALLOCATOR_TYPE::TRACKING: m_allocator = new TrackingAllocator(*m_logger, _config.allocatorDesc.trackingAllocator); break;
		}

		glfwSetErrorCallback(GLFWErrorCallback);
		glfwInit();
		m_logger->IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::CONTEXT, "GLFW Initialised\n");
		
		m_logger->Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::CONTEXT, "Context Initialised\n");
	}



	void Context::Shutdown()
	{
		m_logger->Indent();
		m_logger->Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::CONTEXT, "Shutting Down Context\n");
		
		glfwTerminate();
		m_logger->IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::CONTEXT, "GLFW Terminated\n");
		
		delete m_allocator;
		m_logger->Unindent();
		delete m_logger;
	}

}