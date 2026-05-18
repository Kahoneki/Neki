#include <Core/Engine.h>
#include <Core/EngineEntryPoint.cpp>

#define GLM_ENABLE_EXPERIMENTAL
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
#include <Core/Utils/NTCLoader.h>
#include <Graphics/Lights/DirectionalLight.h>
#include <Graphics/Lights/PointLight.h>
#include <Graphics/Lights/SpotLight.h>
#include <Managers/InputManager.h>
#include <Managers/TimeManager.h>

#include <glm/gtx/string_cast.hpp>



class GameScene final : public NK::Scene
{
public:
	explicit GameScene() : Scene(128)
	{
		// std::filesystem::path serialisedModelOutputPath{ std::filesystem::path(NEKI_SOURCE_DIR) / std::string("Samples/Resource-Files/nkmodels/Prefabs/Plane.nkmodel") };
		// NK::ModelLoader::SerialiseNKModel("Samples/Resource-Files/Prefabs/Plane.gltf", serialisedModelOutputPath.string(), true, true);
		// std::filesystem::path serialisedModelOutputPath{ std::filesystem::path(NEKI_SOURCE_DIR) / std::string("Samples/Resource-Files/nkmodels/SponzaTest/Sponza.nkmodel") };
		// NK::ModelLoader::SerialiseNKModel("Samples/Resource-Files/Sponza/Sponza.gltf", serialisedModelOutputPath.string(), true, true);
		// std::filesystem::path serialisedModelOutputPath{ std::filesystem::path(NEKI_SOURCE_DIR) / std::string("Samples/Resource-Files/nkmodels/Test/DamagedHelmet.nkmodel") };
		// NK::ModelLoader::SerialiseNKModel("Samples/Resource-Files/DamagedHelmet/DamagedHelmet.gltf", serialisedModelOutputPath.string(), true, true);
		// std::filesystem::path serialisedModelOutputPath{ std::filesystem::path(NEKI_SOURCE_DIR) / std::string("Samples/Resource-Files/nkmodels/BistroTest/Bistro.nkmodel") };
		// NK::ModelLoader::SerialiseNKModel("Samples/Resource-Files/Bistro_v5_2/BistroExterior.fbx", serialisedModelOutputPath.string(), true, true);
		
		// std::filesystem::path serialisedModelOutputPath{ std::filesystem::path(NEKI_SOURCE_DIR) / std::string("Samples/Resource-Files/nkmodels/NTCTest/DamagedHelmet.nkmodel") };
		// NK::ModelLoader::SerialiseNKModelNTC("Samples/Resource-Files/DamagedHelmet/DamagedHelmet.gltf", serialisedModelOutputPath.string(), true, true, {2, 16, 3000});
		// std::filesystem::path serialisedModelOutputPath{ std::filesystem::path(NEKI_SOURCE_DIR) / std::string("Samples/Resource-Files/nkmodels/NTCSponzaTest/Sponza.nkmodel") };
		// NK::ModelLoader::SerialiseNKModelNTC("Samples/Resource-Files/Sponza/Sponza.gltf", serialisedModelOutputPath.string(), true, true, {2, 16, 100});
		
		
		m_helmetEntity = m_reg.Create();
		NK::CModelRenderer& helmetModelRenderer{ m_reg.AddComponent<NK::CModelRenderer>(m_helmetEntity) };
		helmetModelRenderer.SetModelPath("Samples/Resource-Files/nkmodels/BistroTest/Bistro.nkmodel");
		NK::CTransform& helmetTransform{ m_reg.GetComponent<NK::CTransform>(m_helmetEntity) };
		helmetTransform.name = "Sponza";
		helmetTransform.SetLocalPosition({ -1.0f, 0.0f, 0.0f });
		helmetTransform.SetLocalRotation({ glm::radians(-90.0f), 0.0f, glm::radians(180.0f) });
		helmetTransform.SetLocalScale({ 0.01, 0.01, 0.01 });
		
		m_helmetEntity2 = m_reg.Create();
		NK::CModelRenderer& helmetModelRenderer2{ m_reg.AddComponent<NK::CModelRenderer>(m_helmetEntity2) };
		helmetModelRenderer2.SetModelPath("Samples/Resource-Files/nkmodels/NTCTest/DamagedHelmet.nkmodel");
		NK::CTransform& helmetTransform2{ m_reg.GetComponent<NK::CTransform>(m_helmetEntity2) };
		helmetTransform2.name = "Helmet";
		helmetTransform2.SetLocalPosition({ 1.0f, 0.0f, 0.0f });
		
		m_skyboxEntity = m_reg.Create();
		NK::CSkybox& skybox{ m_reg.AddComponent<NK::CSkybox>(m_skyboxEntity) };
		m_reg.GetComponent<NK::CTransform>(m_skyboxEntity).name = "Skybox";
		skybox.SetSkyboxFilepath("Samples/Resource-Files/Skyboxes/The Sky is On Fire/skybox.ktx");
		skybox.SetIrradianceFilepath("Samples/Resource-Files/Skyboxes/The Sky is On Fire/irradiance.ktx");
		skybox.SetPrefilterFilepath("Samples/Resource-Files/Skyboxes/The Sky is On Fire/prefilter.ktx");

		// m_lightEntity1 = m_reg.Create();
		// NK::CTransform& directionalLightTransform{ m_reg.GetComponent<NK::CTransform>(m_lightEntity1) };
		// directionalLightTransform.name = "Directional Light";
		// directionalLightTransform.SetLocalRotation({ glm::radians(90.0f), glm::radians(0.0f), glm::radians(0.0f) });
		// directionalLightTransform.SetLocalPosition({ 0.0f, 10.0f, 5.0f });
		// NK::CLight& directionalLight{ m_reg.AddComponent<NK::CLight>(m_lightEntity1) };
		// directionalLight.SetLightType(NK::LIGHT_TYPE::DIRECTIONAL);
		// directionalLight.light->SetColour({ 1,1,1 });
		// directionalLight.light->SetIntensity(1.0f);
		// dynamic_cast<NK::DirectionalLight*>(directionalLight.light.get())->SetDimensions({ 50, 50, 50 });
		
		m_cameraEntity = m_reg.Create();
		NK::CCamera& camera{ m_reg.AddComponent<NK::CCamera>(m_cameraEntity) };
		camera.SetCameraType(NK::CAMERA_TYPE::PLAYER_CAMERA);
		camera.camera->SetNearPlaneDistance(0.01f);
		camera.camera->SetFarPlaneDistance(1000.0f);
		camera.camera->SetFOV(90.0f);
		NK::CTransform& camTransform{ m_reg.GetComponent<NK::CTransform>(m_cameraEntity) };
		camTransform.name = "Camera";
		camTransform.SetLocalPosition({ 0.0f, 3.0f, -5.0f });
		camTransform.SetLocalRotation(glm::vec3(glm::radians(-15.0f), glm::radians(90.0f), 0.0f));


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
		
		
		// //VERY temp
		// NK::Neural::NTCModel* model{ NK::Neural::NTCLoader::LoadMaterial("Resource-Files/model.pt") };
		// helmetModelRenderer.ntcModel = model;
	}
	
	virtual void Update() override {}


private:
	NK::Entity m_helmetEntity;
	NK::Entity m_helmetEntity2;
	NK::Entity m_cameraEntity;
	NK::Entity m_skyboxEntity;
	NK::Entity m_lightEntity1;
};


class GameApp final : public NK::Application
{
public:
	explicit GameApp() : Application(1)
	{
		m_scenes.push_back(NK::UniquePtr<NK::Scene>(NK_NEW(GameScene)));
		m_activeScene = 0;


		//Window
		NK::WindowDesc windowDesc;
		windowDesc.name = "NTCDemo";
		windowDesc.size = { 3840, 2160 };
		m_window = NK::UniquePtr<NK::Window>(NK_NEW(NK::Window, windowDesc));
		m_window->SetCursorVisibility(false);


		//Pre-app layers
		m_windowLayer = NK::UniquePtr<NK::WindowLayer>(NK_NEW(NK::WindowLayer, m_reg));
		NK::InputLayerDesc inputLayerDesc{ m_window.get() };
		m_inputLayer = NK::UniquePtr<NK::InputLayer>(NK_NEW(NK::InputLayer, m_scenes[m_activeScene]->m_reg, inputLayerDesc));
		
		NK::RenderLayerDesc renderLayerDesc{};
		renderLayerDesc.backend = NK::GRAPHICS_BACKEND::VULKAN;
		renderLayerDesc.enableMSAA = false;
		renderLayerDesc.msaaSampleCount = NK::SAMPLE_COUNT::BIT_8;
		renderLayerDesc.enableSSAA = true;
		renderLayerDesc.ssaaMultiplier = 2;
		renderLayerDesc.window = m_window.get();
		renderLayerDesc.renderResolution = glm::ivec2(1920, 1080);
		renderLayerDesc.framesInFlight = 3;
		m_renderLayer = NK::UniquePtr<NK::RenderLayer>(NK_NEW(NK::RenderLayer, m_scenes[m_activeScene]->m_reg, renderLayerDesc));
		
		m_playerCameraLayer = NK::UniquePtr<NK::PlayerCameraLayer>(NK_NEW(NK::PlayerCameraLayer, m_scenes[m_activeScene]->m_reg));
		
		m_preAppLayers.push_back(m_windowLayer.get());
		m_preAppLayers.push_back(m_inputLayer.get());
		m_preAppLayers.push_back(m_renderLayer.get());
		m_preAppLayers.push_back(m_playerCameraLayer.get());
		

		//Post-app layers
		m_postAppLayers.push_back(m_renderLayer.get());
	}



	virtual void Update() override
	{
		const std::size_t oldActiveScene{ m_activeScene };
		
		m_scenes[m_activeScene]->Update();
		
		#if NEKI_EDITOR
			m_window->SetCursorVisibility(NK::Context::GetEditorActive());
		#endif
		
		if (m_activeScene != oldActiveScene)
		{
			for (NK::ILayer* layer : m_preAppLayers)
			{
				layer->SetRegistry(m_scenes[m_activeScene]->m_reg);
			}
			for (NK::ILayer* layer : m_postAppLayers)
			{
				layer->SetRegistry(m_scenes[m_activeScene]->m_reg);
			}
		}
		
		m_shutdown = m_window->ShouldClose();
	}


private:
	NK::UniquePtr<NK::Window> m_window;
	
	//Pre-app layers
	NK::UniquePtr<NK::WindowLayer> m_windowLayer;
	NK::UniquePtr<NK::InputLayer> m_inputLayer;
	NK::UniquePtr<NK::PlayerCameraLayer> m_playerCameraLayer;

	//Post-app layers
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