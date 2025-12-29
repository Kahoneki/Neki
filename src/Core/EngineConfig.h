#pragma once

#include "Application.h"


namespace NK
{
	
	struct EngineConfig
	{
		Application* application;
		
		float fixedUpdateSpeedFactor{ 1.0f }; //Default: 1.0f
	};
	
}