#include "Engine.h"

#include "Context.h"
#include "Debug/ConsoleLogger.h"
#include "Memory/TrackingAllocator.h"

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <GLFW/glfw3.h>


namespace NK
{

	Engine::Engine(const EngineConfig& _config)
	: m_application(UniquePtr<Application>(_config.application))
	{
		//Initialise subsystems
		if (_config.renderSystemDesc.backend != GRAPHICS_BACKEND::NONE)
		{
			m_renderSystem = UniquePtr<RenderSystem>(NK_NEW(RenderSystem, *Context::GetLogger(), *Context::GetAllocator(), _config.renderSystemDesc));
		}
		
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
		//If render system was initialised, start render loop, otherwise, just run once
		if (m_renderSystem)
		{
			while (!m_renderSystem->WindowShouldClose())
			{
				glfwPollEvents();
				m_application->Run();
				m_renderSystem->Update(m_application->m_scenes[m_application->m_activeScene]->m_reg);
			}
		}
		else
		{
			m_application->Run();
		}
	}

}