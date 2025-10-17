#include <Components/CModelRenderer.h>
#include <Core/EngineConfig.h>
#include <Core/RAIIContext.h>


class GameScene final : public NK::Scene
{
public:
	explicit GameScene() : Scene(1)
	{
		m_entity = m_reg.Create();
		m_reg.AddComponent<NK::CModelRenderer>(m_entity);
	}

	
private:
	NK::Entity m_entity;
};


class GameApp final : public NK::Application
{
public:
	explicit GameApp()
	{
		m_scenes.push_back(NK::UniquePtr<NK::Scene>(NK_NEW(GameScene)));
		m_activeScene = 0;
	}

	
	virtual void Run() override {}
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
	renderSystemDesc.backend = NK::GRAPHICS_BACKEND::VULKAN;
	NK::EngineConfig config{ NK_NEW(GameApp), renderSystemDesc };
	return config;
}