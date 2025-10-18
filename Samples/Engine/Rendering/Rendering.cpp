#include <Components/CCamera.h>
#include <Components/CModelRenderer.h>
#include <Components/CTransform.h>
#include <Core/EngineConfig.h>
#include <Core/RAIIContext.h>
#include <Graphics/Camera/PlayerCamera.h>


class GameScene final : public NK::Scene
{
public:
	explicit GameScene() : Scene(2), m_playerCamera(NK::PlayerCamera(glm::vec3(0, 0, 3), -90.0f, 0, 0.01f, 100.0f, 90.0f, 1.0f, 300.0f, 0.05f))
	{
		m_helmetEntity = m_reg.Create();
		NK::CModelRenderer& modelRenderer{ m_reg.AddComponent<NK::CModelRenderer>(m_helmetEntity) };
		modelRenderer.modelPath = "Samples/Resource Files/DamagedHelmet/DamagedHelmet.gltf";
		NK::CTransform& transform{ m_reg.AddComponent<NK::CTransform>(m_helmetEntity) };
		transform.SetPosition(glm::vec3(0,0,-3));
		
		m_cameraEntity = m_reg.Create();
		NK::CCAmera& camera{ m_reg.AddComponent<NK::CCAmera>(m_cameraEntity) };
		camera.camera = &m_playerCamera;
	}


	virtual void Update() override
	{
		m_playerCamera.Update();
	}

	
private:
	NK::Entity m_helmetEntity;
	NK::Entity m_cameraEntity;
	NK::PlayerCamera m_playerCamera;
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
	renderSystemDesc.backend = NK::GRAPHICS_BACKEND::VULKAN;
	NK::EngineConfig config{ NK_NEW(GameApp), renderSystemDesc };
	return config;
}