#pragma once

#include "Application.h"
#include "EngineConfig.h"
#include "Debug/ILogger.h"
#include "Memory/Allocation.h"


namespace NK
{

	class IAllocator;
	
	class Engine final
	{
	public:
		explicit Engine(const EngineConfig& _config);
		~Engine();

		void Run();


	private:
		//Needs to be a regular std::unique_ptr for circular dependency reasons
		std::unique_ptr<ILogger> m_logger;
		
		Application* m_application;

		UniquePtr<RenderSystem> m_renderSystem;
	};
	
}