#define GLM_ENABLE_EXPERIMENTAL
#include <filesystem>
#include <Components/CBoxCollider.h>
#include <Components/CCamera.h>
#include <Components/CInput.h>
#include <Components/CLight.h>
#include <Components/CModelRenderer.h>
#include <Components/CPhysicsBody.h>
#include <Components/CSkybox.h>
#include <Components/CTransform.h>
#include <Core/EngineConfig.h>
#include <Core/RAIIContext.h>
#include <Core/Layers/InputLayer.h>
#include <Core/Layers/ModelVisibilityLayer.h>
#include <Core/Layers/PhysicsLayer.h>
#include <Core/Layers/PlayerCameraLayer.h>
#include <Core/Layers/RenderLayer.h>
#include <Core/Layers/WindowLayer.h>
#include <Graphics/Camera/PlayerCamera.h>
#include <Graphics/Lights/DirectionalLight.h>
#include <Graphics/Lights/PointLight.h>
#include <Graphics/Lights/SpotLight.h>
#include <Managers/InputManager.h>
#include <Managers/TimeManager.h>

#include <imgui.h>
#include <glm/gtx/string_cast.hpp>




static const NK::PhysicsBroadPhaseLayer movingBroadPhaseLayer{ 0 };
static const NK::PhysicsBroadPhaseLayer staticBroadPhaseLayer{ 1 };
static const NK::PhysicsObjectLayer helmetObjectLayer{ 0, movingBroadPhaseLayer };
static const NK::PhysicsObjectLayer floorObjectLayer{ 1, staticBroadPhaseLayer };


class GameScene final : public NK::Scene
{
public:
	explicit GameScene() : Scene(5), m_playerCamera(NK::UniquePtr<NK::PlayerCamera>(NK_NEW(NK::PlayerCamera, glm::vec3(0.0f, 3.0f, -5.0f), 90.0f, -15.0f, 0.01f, 1000.0f, 90.0f, WIN_ASPECT_RATIO, 30.0f, 0.05f)))
	{
		//preprocessing step - ONLY NEEDS TO BE CALLED ONCE - serialises the model into a persistent .nkmodel file that can then be loaded
		//		std::filesystem::path serialisedModelOutputPath{ std::filesystem::path(NEKI_SOURCE_DIR) / std::string("Samples/Resource-Files/nkmodels/DamagedHelmet/DamagedHelmet.nkmodel") };
		//		NK::ModelLoader::SerialiseNKModel("Samples/Resource-Files/DamagedHelmet/DamagedHelmet.gltf", serialisedModelOutputPath.string(), true, true);
		//		serialisedModelOutputPath = std::filesystem::path(NEKI_SOURCE_DIR) / std::string("Samples/Resource-Files/nkmodels/Prefabs/Cube.nkmodel");
		//		NK::ModelLoader::SerialiseNKModel("Samples/Resource-Files/Prefabs/Cube.gltf", serialisedModelOutputPath.string(), true, true);


		m_skyboxEntity = m_reg.Create();
		NK::CSkybox& skybox{ m_reg.AddComponent<NK::CSkybox>(m_skyboxEntity) };
		skybox.SetSkyboxFilepath("Samples/Resource-Files/Skyboxes/The Sky is On Fire/skybox.ktx");
		skybox.SetIrradianceFilepath("Samples/Resource-Files/Skyboxes/The Sky is On Fire/irradiance.ktx");
		skybox.SetPrefilterFilepath("Samples/Resource-Files/Skyboxes/The Sky is On Fire/prefilter.ktx");

		m_lightEntity = m_reg.Create();
		NK::CTransform& directionalLightTransform{ m_reg.AddComponent<NK::CTransform>(m_lightEntity) };
		directionalLightTransform.SetRotation({ glm::radians(95.2f), glm::radians(54.3f), glm::radians(-24.6f) });
		directionalLightTransform.SetPosition({ 0.0f, 10.0f, 5.0f });
		NK::CLight& directionalLight{ m_reg.AddComponent<NK::CLight>(m_lightEntity) };
		directionalLight.lightType = NK::LIGHT_TYPE::DIRECTIONAL;
		directionalLight.light = NK::UniquePtr<NK::Light>(NK_NEW(NK::DirectionalLight));
		directionalLight.light->SetColour({ 1,0,0 });
		directionalLight.light->SetIntensity(10.0f);
		dynamic_cast<NK::DirectionalLight*>(directionalLight.light.get())->SetDimensions({ 50, 50, 50 });

		m_floorEntity = m_reg.Create();
		NK::CModelRenderer& floorModelRenderer{ m_reg.AddComponent<NK::CModelRenderer>(m_floorEntity) };
		floorModelRenderer.modelPath = "Samples/Resource-Files/nkmodels/Prefabs/Cube.nkmodel";
		NK::CTransform& floorTransform{ m_reg.AddComponent<NK::CTransform>(m_floorEntity) };
		floorTransform.SetPosition({ 0, 0.0f, 0.0f });
		floorTransform.SetScale({ 5.0f, 0.2f, 5.0f });
//		NK::CPhysicsBody& floorPhysicsBody{ m_reg.AddComponent<NK::CPhysicsBody>(m_floorEntity) };
//		floorPhysicsBody.dynamic = false;
//		floorPhysicsBody.objectLayer = floorObjectLayer;
		NK::CBoxCollider floorCollider{ m_reg.AddComponent<NK::CBoxCollider>(m_floorEntity) };
		floorCollider.halfExtents = { 5.0f, 0.2f, 5.0f };
		m_helmetEntity = m_reg.Create();
		NK::CModelRenderer& helmetModelRenderer{ m_reg.AddComponent<NK::CModelRenderer>(m_helmetEntity) };
		helmetModelRenderer.modelPath = "Samples/Resource-Files/nkmodels/DamagedHelmet/DamagedHelmet.nkmodel";
		NK::CTransform& helmetTransform{ m_reg.AddComponent<NK::CTransform>(m_helmetEntity) };
		helmetTransform.SetPosition({ 0, 10.0f, 0.0f });
		helmetTransform.SetScale({ 1.0f, 1.0f, 1.0f });
		helmetTransform.SetRotation({ glm::radians(70.0f), glm::radians(-30.0f), glm::radians(180.0f) });
//		NK::CPhysicsBody& helmetPhysicsBody{ m_reg.AddComponent<NK::CPhysicsBody>(m_helmetEntity) };
//		helmetPhysicsBody.dynamic = true;
//		helmetPhysicsBody.objectLayer = helmetObjectLayer;
		NK::CBoxCollider helmetCollider{ m_reg.AddComponent<NK::CBoxCollider>(m_helmetEntity) };
		helmetCollider.halfExtents = { 0.944977, 1.000000, 0.900984 };
		


		m_cameraEntity = m_reg.Create();
		NK::CCamera& camera{ m_reg.AddComponent<NK::CCamera>(m_cameraEntity) };
		camera.camera = m_playerCamera.get();


		//Inputs
		NK::ButtonBinding aBinding{ NK::KEYBOARD::A };
		NK::ButtonBinding dBinding{ NK::KEYBOARD::D };
		NK::ButtonBinding sBinding{ NK::KEYBOARD::S };
		NK::ButtonBinding wBinding{ NK::KEYBOARD::W };
		NK::Axis1DBinding camMoveHorizontalBinding{ { aBinding, dBinding }, { -1, 1 } };
		NK::Axis1DBinding camMoveVerticalBinding{ { sBinding, wBinding }, { -1, 1 } };
		NK::Axis2DBinding camMoveBinding{ NK::Axis2DBinding({ camMoveHorizontalBinding, camMoveVerticalBinding }) };
		NK::Axis2DBinding mouseDiffBinding{ NK::Axis2DBinding(NK::MOUSE::POSITION_DIFFERENCE) };
		NK::InputManager::BindActionToInput(NK::PLAYER_CAMERA_ACTIONS::MOVE, camMoveBinding);
		NK::InputManager::BindActionToInput(NK::PLAYER_CAMERA_ACTIONS::YAW_PITCH, mouseDiffBinding);

		NK::CInput& input{ m_reg.AddComponent<NK::CInput>(m_cameraEntity) };
		input.AddActionToMap(NK::PLAYER_CAMERA_ACTIONS::MOVE);
		input.AddActionToMap(NK::PLAYER_CAMERA_ACTIONS::YAW_PITCH);
	}



	virtual void Update() override
	{
//		NK::CTransform& helmetTransform{ m_reg.GetComponent<NK::CTransform>(m_helmetEntity) };
//		constexpr float speed{ 50.0f };
//		const float rotationAmount{ glm::radians(speed * static_cast<float>(NK::TimeManager::GetDeltaTime())) };
//		helmetTransform.SetRotation(helmetTransform.GetRotation() + glm::vec3(0, rotationAmount, 0));
	}


private:
	NK::Entity m_skyboxEntity;
	NK::Entity m_helmetEntity;
	NK::Entity m_floorEntity;
	NK::Entity m_lightEntity;
	NK::Entity m_cameraEntity;
	NK::UniquePtr<NK::PlayerCamera> m_playerCamera;
};


class GameApp final : public NK::Application
{
public:
	explicit GameApp() : Application(1)
	{
		//Register types
		NK::TypeRegistry::Register<NK::PLAYER_CAMERA_ACTIONS>("PLAYER_CAMERA_ACTIONS");

		m_scenes.push_back(NK::UniquePtr<NK::Scene>(NK_NEW(GameScene)));
		m_activeScene = 0;


		//Window
		NK::WindowDesc windowDesc;
		windowDesc.name = "Rendering Sample";
		windowDesc.size = { 1920, 1080 };
		m_window = NK::UniquePtr<NK::Window>(NK_NEW(NK::Window, windowDesc));
		m_window->SetCursorVisibility(false);

		m_windowEntity = m_reg.Create();
		NK::CWindow& windowComponent{ m_reg.AddComponent<NK::CWindow>(m_windowEntity) };
		windowComponent.window = m_window.get();


		//Pre-app layers
		m_windowLayer = NK::UniquePtr<NK::WindowLayer>(NK_NEW(NK::WindowLayer, m_reg));
		NK::InputLayerDesc inputLayerDesc{ m_window.get() };
		m_inputLayer = NK::UniquePtr<NK::InputLayer>(NK_NEW(NK::InputLayer, m_scenes[m_activeScene]->m_reg, inputLayerDesc));
		
		NK::RenderLayerDesc renderLayerDesc{};
		renderLayerDesc.backend = NK::GRAPHICS_BACKEND::VULKAN;
		renderLayerDesc.enableMSAA = false;
		renderLayerDesc.msaaSampleCount = NK::SAMPLE_COUNT::BIT_8;
		renderLayerDesc.enableSSAA = true;
		renderLayerDesc.ssaaMultiplier = 4;
		renderLayerDesc.window = m_window.get();
		renderLayerDesc.framesInFlight = 3;
		m_renderLayer = NK::UniquePtr<NK::RenderLayer>(NK_NEW(NK::RenderLayer, m_scenes[m_activeScene]->m_reg, renderLayerDesc));
		
		m_playerCameraLayer = NK::UniquePtr<NK::PlayerCameraLayer>(NK_NEW(NK::PlayerCameraLayer, m_scenes[m_activeScene]->m_reg));
		
		m_preAppLayers.push_back(m_windowLayer.get());
		m_preAppLayers.push_back(m_inputLayer.get());
		m_preAppLayers.push_back(m_renderLayer.get());
		m_preAppLayers.push_back(m_playerCameraLayer.get());
		

		//Post-app layers
		NK::PhysicsLayerDesc physicsLayerDesc{};
		physicsLayerDesc.objectLayers = { helmetObjectLayer, floorObjectLayer };
		physicsLayerDesc.objectLayerCollisionPartners = { { helmetObjectLayer, {helmetObjectLayer, floorObjectLayer} }, { floorObjectLayer, { floorObjectLayer } } };
		m_physicsLayer = NK::UniquePtr<NK::PhysicsLayer>(NK_NEW(NK::PhysicsLayer, m_scenes[m_activeScene]->m_reg, physicsLayerDesc));
		
		m_postAppLayers.push_back(m_physicsLayer.get());
		m_postAppLayers.push_back(m_renderLayer.get());
	}



	virtual void Update() override
	{
		m_scenes[m_activeScene]->Update();

		glfwSetWindowShouldClose(m_window->GetGLFWWindow(), NK::InputManager::GetKeyPressed(NK::KEYBOARD::ESCAPE));
		if (NK::InputManager::GetKeyPressed(NK::KEYBOARD::E) && !m_editorActiveKeyPressedLastFrame)
		{
			NK::Context::SetEditorActive(!NK::Context::GetEditorActive());
		}
		m_editorActiveKeyPressedLastFrame = NK::InputManager::GetKeyPressed(NK::KEYBOARD::E);

		m_window->SetCursorVisibility(NK::Context::GetEditorActive());

		m_shutdown = m_window->ShouldClose();
	}


private:
	NK::UniquePtr<NK::Window> m_window;
	NK::Entity m_windowEntity;

	bool m_editorActiveKeyPressedLastFrame{ false };

	//Pre-app layers
	NK::UniquePtr<NK::WindowLayer> m_windowLayer;
	NK::UniquePtr<NK::InputLayer> m_inputLayer;
	NK::UniquePtr<NK::PlayerCameraLayer> m_playerCameraLayer;
	NK::UniquePtr<NK::PhysicsLayer> m_physicsLayer;

	//Post-app layers
	NK::UniquePtr<NK::ModelVisibilityLayer> m_modelVisibilityLayer;
	NK::UniquePtr<NK::RenderLayer> m_renderLayer;
};



[[nodiscard]] NK::ContextConfig CreateContext()
{
	NK::LoggerConfig loggerConfig{ NK::LOGGER_TYPE::CONSOLE, true };
	loggerConfig.SetLayerChannelBitfield(NK::LOGGER_LAYER::VULKAN_GENERAL, NK::LOGGER_CHANNEL::WARNING | NK::LOGGER_CHANNEL::ERROR);
	loggerConfig.SetLayerChannelBitfield(NK::LOGGER_LAYER::VULKAN_VALIDATION, NK::LOGGER_CHANNEL::INFO | NK::LOGGER_CHANNEL::WARNING | NK::LOGGER_CHANNEL::ERROR);
	loggerConfig.SetLayerChannelBitfield(NK::LOGGER_LAYER::TRACKING_ALLOCATOR, NK::LOGGER_CHANNEL::WARNING | NK::LOGGER_CHANNEL::ERROR);

	constexpr NK::TrackingAllocatorConfig trackingAllocatorConfig{ NK::TRACKING_ALLOCATOR_VERBOSITY_FLAGS::ALL };
	constexpr NK::AllocatorConfig allocatorConfig{ NK::ALLOCATOR_TYPE::TRACKING, trackingAllocatorConfig };

	return NK::ContextConfig(loggerConfig, allocatorConfig);
}



[[nodiscard]] NK::EngineConfig CreateEngine()
{
	return NK::EngineConfig(NK_NEW(GameApp));
}