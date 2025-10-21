#pragma once

#include "Application.h"
#include "Subsystems/NetworkSystem.h"
#include "Subsystems/RenderSystem.h"


namespace NK
{
	
	struct EngineConfig
	{
		Application* application;

		RenderSystemDesc renderSystemDesc{};
		NetworkSystemDesc networkSystemDesc{};
	};
	
}