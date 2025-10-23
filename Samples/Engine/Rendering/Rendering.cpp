#include <Components/CCamera.h>
#include <Components/CModelRenderer.h>
#include <Components/CSkybox.h>
#include <Components/CTransform.h>
#include <Core/EngineConfig.h>
#include <Core/RAIIContext.h>
#include <Graphics/Camera/PlayerCamera.h>

#include "Core/Layers/RenderLayer.h"
#include "Managers/InputManager.h"
#include "Managers/TimeManager.h"


class GameScene final : public NK::Scene
{
public:
	explicit GameScene() : Scene(3), m_playerCamera(NK::PlayerCamera(glm::vec3(0, 0, 3), -90.0f, 0, 0.01f, 100.0f, 90.0f, WIN_ASPECT_RATIO, 30.0f, 0.05f))
	{
		m_skyboxEntity = m_reg.Create();
		NK::CSkybox& skybox{ m_reg.AddComponent<NK::CSkybox>(m_skyboxEntity) };
		skybox.SetTextureDirectory("Samples/Resource Files/skybox");
		skybox.SetFileExtension("jpg");

		m_helmetEntity = m_reg.Create();
		NK::CModelRenderer& modelRenderer{ m_reg.AddComponent<NK::CModelRenderer>(m_helmetEntity) };
		modelRenderer.modelPath = "Samples/Resource Files/DamagedHelmet/DamagedHelmet.gltf";
		NK::CTransform& transform{ m_reg.AddComponent<NK::CTransform>(m_helmetEntity) };
		transform.SetPosition(glm::vec3(0, 0, -3));
		transform.SetRotation({ glm::radians(70.0f), glm::radians(-30.0f), glm::radians(180.0f) });

		m_cameraEntity = m_reg.Create();
		NK::CCamera& camera{ m_reg.AddComponent<NK::CCamera>(m_cameraEntity) };
		camera.camera = &m_playerCamera;
	}



	virtual void Update() override
	{
		m_playerCamera.Update();
		NK::CTransform& transform{ m_reg.GetComponent<NK::CTransform>(m_helmetEntity) };
		constexpr float speed{ 50.0f };
		const float rotationAmount{ glm::radians(speed * static_cast<float>(NK::TimeManager::GetDeltaTime())) };
		transform.SetRotation(transform.GetRotation() + glm::vec3(0, rotationAmount, 0));
	}


private:
	NK::Entity m_skyboxEntity;
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

		NK::RenderLayerDesc renderLayerDesc{};
		renderLayerDesc.backend = NK::GRAPHICS_BACKEND::VULKAN;
		renderLayerDesc.enableSSAA = true;
		renderLayerDesc.ssaaMultiplier = 4;
		renderLayerDesc.windowDesc.size = { 1920, 1080 };
		m_postAppLayers.push_back(NK::UniquePtr<NK::ILayer>(NK_NEW(NK::RenderLayer, renderLayerDesc)));
	}



	virtual void Update() override
	{
		NK::InputManager::Update(dynamic_cast<NK::RenderLayer*>(m_postAppLayers[0].get())->GetWindow());
		m_scenes[m_activeScene]->Update();
		m_shutdown = dynamic_cast<NK::RenderLayer*>(m_postAppLayers[0].get())->GetWindow()->ShouldClose();
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
	const NK::EngineConfig config{ NK_NEW(GameApp) };
	return config;
}