#include "Engine.h"
#include "EngineConfig.h"


//To be implemented by user
[[nodiscard]] extern NK::EngineConfig CreateEngine();



int main()
{
	const NK::EngineConfig config{ CreateEngine() };
	NK::Engine engine{ config };

	engine.Run();
	
	delete config.application;
}