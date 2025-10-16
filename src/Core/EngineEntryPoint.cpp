#include "Engine.h"
#include "EngineConfig.h"
#include "RAIIContext.h"


//To be implemented by user
[[nodiscard]] extern NK::EngineConfig CreateEngine();



int main()
{
	const NK::EngineConfig config{ CreateEngine() };
	NK::RAIIContext context{ config.loggerConfig, config.allocatorType };
	NK::Engine engine{ config.application };

	engine.Run();
	
	delete config.application;
}