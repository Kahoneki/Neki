#pragma once

#include "Application.h"
#include "Subsystems/RenderSystem.h"


namespace NK
{
	
	struct EngineConfig
	{
		Application* application;

		RenderSystemDesc renderSystemDesc;
	};
	
}