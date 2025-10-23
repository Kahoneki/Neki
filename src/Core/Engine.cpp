#include "Engine.h"

#include "Context.h"
#include "Debug/ConsoleLogger.h"
#include "Memory/TrackingAllocator.h"

#include <Managers/InputManager.h>
#include <Managers/TimeManager.h>

#include <stdexcept>
#include <GLFW/glfw3.h>



namespace NK
{

	Engine::Engine(const EngineConfig& _config)
	: m_application(UniquePtr<Application>(_config.application))
	{
		Context::GetLogger()->Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::ENGINE, "Engine Initialised\n");
	}


	
	Engine::~Engine()
	{
		Context::GetLogger()->Indent();
		Context::GetLogger()->Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::ENGINE, "Shutting Down Engine\n");
		Context::GetLogger()->Unindent();
	}



	void Engine::Run()
	{
		while (!m_application->m_shutdown)
		{
			glfwPollEvents();
			TimeManager::Update();
			m_application->PreUpdate();
			m_application->Update();
			m_application->PostUpdate();
		}
	}

}