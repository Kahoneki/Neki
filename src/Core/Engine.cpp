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
	: m_application(UniquePtr<Application>(_config.application)), m_fixedUpdateSpeedFactor(_config.fixedUpdateSpeedFactor), m_timestepAccumulator(0.0f)
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
		bool firstFrame{ true };
		while (!m_application->m_shutdown)
		{
			TimeManager::Update();
			if (firstFrame)
			{
				firstFrame = false;
			}
			else
			{
				m_timestepAccumulator += TimeManager::GetDeltaTime() * m_fixedUpdateSpeedFactor;
			}
			
			while (m_timestepAccumulator >= Context::GetFixedUpdateTimestep())
			{
				Context::SetLayerUpdateState(LAYER_UPDATE_STATE::PRE_APP);
				m_application->PreFixedUpdate();
				m_application->FixedUpdate();
				Context::SetLayerUpdateState(LAYER_UPDATE_STATE::POST_APP);
				m_application->PostFixedUpdate();
				m_timestepAccumulator -= Context::GetFixedUpdateTimestep();
			}
			
			Context::SetLayerUpdateState(LAYER_UPDATE_STATE::PRE_APP);
			m_application->PreUpdate();
			m_application->Update();
			Context::SetLayerUpdateState(LAYER_UPDATE_STATE::POST_APP);
			m_application->PostUpdate();
		}
	}

}