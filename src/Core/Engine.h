#pragma once

#include "Application.h"
#include "EngineConfig.h"
#include "Debug/ILogger.h"
#include "Memory/IAllocator.h"


namespace NK
{

	class Engine final
	{
	public:
		explicit Engine(const EngineConfig& _config);
		~Engine();

		void Run();


	private:
		ILogger* m_logger;
		IAllocator* m_allocator;

		Application* m_application;
		
		UniquePtr<RenderSystem> m_renderSystem;
	};
	
}