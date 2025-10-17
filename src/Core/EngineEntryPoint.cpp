#include "Engine.h"
#include "RAIIContext.h"


//To be implemented by user
[[nodiscard]] extern NK::ContextConfig CreateContext();
[[nodiscard]] extern NK::EngineConfig CreateEngine();



int main()
{
	const NK::ContextConfig contextConfig{ CreateContext() };
	NK::RAIIContext context{ contextConfig };
	
	const NK::EngineConfig engineConfig{ CreateEngine() };
	NK::Engine engine{ engineConfig };
	
	engine.Run();
}