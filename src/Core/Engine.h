#pragma once

#include "Application.h"
#include "EngineConfig.h"
#include "Layers/ILayer.h"
#include "Memory/Allocation.h"


namespace NK
{

	class Engine final
	{
	public:
		explicit Engine(const EngineConfig& _config);
		~Engine();

		void Run();


	private:
		UniquePtr<Application> m_application;
		float m_timestepAccumulator;
		float m_fixedUpdateSpeedFactor;
	};
	
}