#include "Engine.h"

#include "Debug/ConsoleLogger.h"
#include "Memory/TrackingAllocator.h"

#include <stdexcept>
#include <GLFW/glfw3.h>


namespace NK
{

	IAllocator* PrivateAllocator::m_allocator{ nullptr };
	
	
	Engine::Engine(const EngineConfig& _config)
	: m_application(_config.application)
	{
		//Initialise logger
		switch (_config.loggerConfig.type)
		{
		case LOGGER_TYPE::CONSOLE: m_logger = std::make_unique<ConsoleLogger>(ConsoleLogger(_config.loggerConfig)); break;
		default: throw std::invalid_argument("Engine::Engine() - _config.loggerConfig.type not recognised.\n");
		}

		
		//Initialise allocator
		switch (_config.allocatorType)
		{
		case ALLOCATOR_TYPE::TRACKING: PrivateAllocator::m_allocator = new TrackingAllocator(*m_logger, false, false); break;
		case ALLOCATOR_TYPE::TRACKING_VERBOSE: PrivateAllocator::m_allocator = new TrackingAllocator(*m_logger, true, false); break;
		case ALLOCATOR_TYPE::TRACKING_VERBOSE_INCLUDE_VULKAN: PrivateAllocator::m_allocator = new TrackingAllocator(*m_logger, true, true); break;
		default: throw std::invalid_argument("Engine::Engine() - _config.allocatorType not recognised.\n");
		}
		

		//Initialise GLFW
		glfwInit();
		m_logger->Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::ENGINE, "GLFW Initialised\n");


		//Initialise subsystems
		m_renderSystem = UniquePtr<RenderSystem>(NK_NEW(RenderSystem, *m_logger, *PrivateAllocator::m_allocator, _config.renderSystemDesc));
		
		
		m_logger->Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::ENGINE, "Engine Initialised\n");
	}


	
	Engine::~Engine()
	{
		m_logger->Indent();
		m_logger->Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::ENGINE, "Shutting Down Engine\n");
		
		glfwTerminate();
		m_logger->IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::ENGINE, "GLFW Terminated\n");
		
		m_logger->Unindent();
	}



	void Engine::Run()
	{
		m_application->Run();
	}

}