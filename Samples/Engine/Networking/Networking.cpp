#include <Core/EngineConfig.h>

#include <iostream>


class GameScene final : public NK::Scene
{
public:
	explicit GameScene() : Scene(0) {}
	virtual void Update() override {}
};



class GameApp final : public NK::Application
{
public:
	explicit GameApp()
	{
		m_scenes.push_back(NK::UniquePtr<NK::Scene>(NK_NEW(GameScene)));
		m_activeScene = 0;
	}



	virtual void Update() override
	{
		Application::Update();
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
	NK::NetworkSystemDesc networkSystemDesc{};
	networkSystemDesc.server.maxClients = 1;
	networkSystemDesc.server.type = NK::SERVER_TYPE::WAN;
	NK::EngineConfig config;
	config.application = NK_NEW(GameApp);
	config.renderSystemDesc = renderSystemDesc;
	config.networkSystemDesc = networkSystemDesc;
	return config;
}