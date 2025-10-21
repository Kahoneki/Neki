#pragma once

#include "Application.h"
#include "EngineConfig.h"
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
		UniquePtr<Application> m_application;
		UniquePtr<RenderSystem> m_renderSystem;
		UniquePtr<NetworkSystem> m_networkSystem;
	};
	
}