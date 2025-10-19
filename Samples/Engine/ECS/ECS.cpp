#include <Core-ECS/ComponentView.h>
#include <Core-ECS/Registry.h>
#include <Core/EngineConfig.h>

#include <iomanip>
#include <iostream>


class GameApp final : public NK::Application
{
public:
	struct C1 { int x; };
	struct C2 {};
	struct C3 {};

	
	virtual void Update() override
	{
		constexpr int testWidth{ 40 };
		constexpr int resultWidth{ 30 };
		const std::string COLOUR_GREEN{ "\033[32m" };
		const std::string COLOUR_RED{ "\033[31m" };
		const std::string COLOUR_RESET{ "\033[0m" };

		#define SUCC_FAIL(BOOL) ((BOOL) ? (COLOUR_GREEN + std::string("SUCCESS")) : (COLOUR_RED + std::string("FAILURE"))) << COLOUR_RESET
		
		NK::Registry reg{ 3 };


		//Testing Create()
		const NK::Entity e1{ reg.Create() };
		std::cout << std::left << std::setw(testWidth) << "Should be 0:" << std::setw(resultWidth) << e1 << SUCC_FAIL(e1 == 0) << '\n';
		std::cout << std::left << std::setw(testWidth) << "Should be false:" << std::setw(resultWidth) << std::boolalpha << reg.HasComponent<C1>(e1) << SUCC_FAIL(reg.HasComponent<C1>(e1) == false) << '\n';


		//Testing AddComponent<>()
		C1 c1{};
		c1.x = 5;
		reg.AddComponent<C1>(e1, c1);
		std::cout << std::left << std::setw(testWidth) << "Should be true:" << std::setw(resultWidth) << std::boolalpha << reg.HasComponent<C1>(e1) << SUCC_FAIL(reg.HasComponent<C1>(e1) == true) << '\n';
		std::cout << std::left << std::setw(testWidth) << "Should be 5:" << std::setw(resultWidth) << reg.GetComponent<C1>(e1).x << SUCC_FAIL(reg.GetComponent<C1>(e1).x == 5) << '\n';

		
		//Testing GetComponent<>()
		reg.GetComponent<C1>(e1).x = 6;
		std::cout << std::left << std::setw(testWidth) << "Should be 6:" << std::setw(resultWidth) << reg.GetComponent<C1>(e1).x << SUCC_FAIL(reg.GetComponent<C1>(e1).x == 6) << '\n';

		
		//Testing View<>() (access and modification)
		std::cout << std::left << std::setw(testWidth) << "Should be 6:";
		for (auto&& [c1Local] : reg.View<C1>())
		{
			std::cout << std::setw(resultWidth) << c1Local.x << SUCC_FAIL(c1Local.x == 6);
			c1Local.x = 5;
		}
		std::cout << '\n';
		std::cout << std::left << std::setw(testWidth) << "Should be 5:" << std::setw(resultWidth) << reg.GetComponent<C1>(e1).x << SUCC_FAIL(reg.GetComponent<C1>(e1).x == 5) << '\n';

		
		//Testing RemoveComponent<>()
		reg.RemoveComponent<C1>(e1);
		std::cout << std::left << std::setw(testWidth) << "Should be false:" << std::setw(resultWidth) << std::boolalpha << reg.HasComponent<C1>(e1) << SUCC_FAIL(reg.HasComponent<C1>(e1) == false) << '\n';

		
		//Testing Create() when another entity already exists
		const NK::Entity e2{ reg.Create() };
		std::cout << std::left << std::setw(testWidth) << "Should be 1:" << std::setw(resultWidth) << e2 << SUCC_FAIL(e2 == 1) << '\n';

		
		//Testing Destroy()
		reg.Destroy(e1);


		//Testing Create()'s free list allocation logic when an entity index has been freed up
		const NK::Entity e3{ reg.Create() };
		std::cout << std::left << std::setw(testWidth) << "Should be 0:" << std::setw(resultWidth) << e3 << SUCC_FAIL(e3 == 0) << '\n';


		//Testing AddComponent<> on multiple entities
		reg.AddComponent<C1>(e2);
		reg.AddComponent<C2>(e2);
		reg.AddComponent<C2>(e3);
		reg.AddComponent<C3>(e3);
		
		std::string resultStr{ reg.HasComponent<C1>(e2) ? "true, " : "false, " };
		resultStr += reg.HasComponent<C2>(e2) ? "true, " : "false, ";
		resultStr += reg.HasComponent<C3>(e2) ? "true" : "false";
		std::cout << std::left << std::setw(testWidth) << "Should be true, true, false:" << std::setw(resultWidth) << resultStr << SUCC_FAIL(reg.HasComponent<C1>(e2) && reg.HasComponent<C2>(e2) && !reg.HasComponent<C3>(e2)) << '\n';

		resultStr = reg.HasComponent<C1>(e3) ? "true, " : "false, ";
		resultStr += reg.HasComponent<C2>(e3) ? "true, " : "false, ";
		resultStr += reg.HasComponent<C3>(e3) ? "true" : "false";
		std::cout << std::left << std::setw(testWidth) << "Should be false, true, true:" << std::setw(resultWidth) << resultStr << SUCC_FAIL(reg.HasComponent<C1>(e2) && reg.HasComponent<C2>(e2) && !reg.HasComponent<C3>(e2)) << '\n';


		//Testing different View<>() cases
		std::uint32_t counter{ 0 };
		for (const auto&& [c] : reg.View<C1>()) { ++counter; }
		std::cout << std::left << std::setw(testWidth) << "Should be 1:" << std::setw(resultWidth) << counter << SUCC_FAIL(counter == 1) << '\n';
		counter = 0;
		
		for (const auto&& [c] : reg.View<C2>()) { ++counter; }
		std::cout << std::left << std::setw(testWidth) << "Should be 2:" << std::setw(resultWidth) << counter << SUCC_FAIL(counter == 2) << '\n';
		counter = 0;

		for (const auto&& [c] : reg.View<C3>()) { ++counter; }
		std::cout << std::left << std::setw(testWidth) << "Should be 1:" << std::setw(resultWidth) << counter << SUCC_FAIL(counter == 1) << '\n';
		counter = 0;

		for (const auto&& [c, cc] : reg.View<C1, C2>()) { ++counter; }
		std::cout << std::left << std::setw(testWidth) << "Should be 1:" << std::setw(resultWidth) << counter << SUCC_FAIL(counter == 1) << '\n';
		counter = 0;

		for (const auto&& [c, cc] : reg.View<C2, C3>()) { ++counter; }
		std::cout << std::left << std::setw(testWidth) << "Should be 1:" << std::setw(resultWidth) << counter << SUCC_FAIL(counter == 1) << '\n';
		counter = 0;

		for (const auto&& [c, cc, ccc] : reg.View<C1, C2, C3>()) { ++counter; }
		std::cout << std::left << std::setw(testWidth) << "Should be 0:" << std::setw(resultWidth) << counter << SUCC_FAIL(counter == 0) << '\n';
		counter = 0;

		for (const auto&& [c, cc] : reg.View<C1, C3>()) { ++counter; }
		std::cout << std::left << std::setw(testWidth) << "Should be 0:" << std::setw(resultWidth) << counter << SUCC_FAIL(counter == 0) << '\n';
		counter = 0;

		reg.AddComponent<C3>(e2);
		for (const auto&& [c, cc] : reg.View<C1, C3>()) { ++counter; }
		std::cout << std::left << std::setw(testWidth) << "Should be 1:" << std::setw(resultWidth) << counter << SUCC_FAIL(counter == 1) << '\n';
		counter = 0;


		//Testing Registry's shutdown logic
	}
};



[[nodiscard]] NK::ContextConfig CreateContext()
{
	NK::LoggerConfig loggerConfig{ NK::LOGGER_TYPE::CONSOLE, true };
	loggerConfig.SetLayerChannelBitfield(NK::LOGGER_LAYER::VULKAN_GENERAL, NK::LOGGER_CHANNEL::WARNING | NK::LOGGER_CHANNEL::ERROR);
	loggerConfig.SetLayerChannelBitfield(NK::LOGGER_LAYER::VULKAN_VALIDATION, NK::LOGGER_CHANNEL::WARNING | NK::LOGGER_CHANNEL::ERROR);
	loggerConfig.SetLayerChannelBitfield(NK::LOGGER_LAYER::TRACKING_ALLOCATOR, NK::LOGGER_CHANNEL::WARNING | NK::LOGGER_CHANNEL::ERROR);
	const NK::ContextConfig config{ loggerConfig, NK::ALLOCATOR_TYPE::TRACKING_VERBOSE };
	return config;
}



[[nodiscard]] NK::EngineConfig CreateEngine()
{
	NK::RenderSystemDesc renderSystemDesc{};
	renderSystemDesc.backend = NK::GRAPHICS_BACKEND::NONE;
	NK::EngineConfig config{ NK_NEW(GameApp), renderSystemDesc };
	return config;
}