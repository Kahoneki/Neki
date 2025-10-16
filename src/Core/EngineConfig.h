#pragma once

#include "Application.h"
#include "Debug/LoggerConfig.h"
#include "Subsystems/RenderSystem.h"


namespace NK
{
	
	struct EngineConfig
	{
		LoggerConfig loggerConfig;
		ALLOCATOR_TYPE allocatorType;
		Application* application;

		RenderSystemDesc renderSystemDesc;
	};
	
}