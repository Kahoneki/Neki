#include <filesystem>
#include <Components/CCamera.h>
#include <Components/CInput.h>
#include <Components/CModelRenderer.h>
#include <Components/CSkybox.h>
#include <Components/CTransform.h>
#include <Core/EngineConfig.h>
#include <Core/RAIIContext.h>
#include <Core/Layers/InputLayer.h>
#include <Core/Layers/ModelVisibilityLayer.h>
#include <Core/Layers/PlayerCameraLayer.h>
#include <Core/Layers/RenderLayer.h>
#include <Core/Layers/WindowLayer.h>
#include <Graphics/Camera/PlayerCamera.h>
#include <Managers/InputManager.h>
#include <Managers/TimeManager.h>


class GameScene final : public NK::Scene
{
public:
	explicit GameScene() : Scene(3), m_playerCamera(NK::UniquePtr<NK::PlayerCamera>(NK_NEW(NK::PlayerCamera, glm::vec3(0, 0, 3), -90.0f, 0, 0.01f, 100.0f, 90.0f, WIN_ASPECT_RATIO, 30.0f, 0.05f)))
	{
		m_skyboxEntity = m_reg.Create();
		NK::CSkybox& skybox{ m_reg.AddComponent<NK::CSkybox>(m_skyboxEntity) };
		skybox.SetTextureDirectory("Samples/Resource-Files/skybox");
		skybox.SetFileExtension("jpg");

		//ONLY NEEDS TO BE CALLED ONCE - serialises the model into a persistent .nkmodel file that can then be loaded
//		std::filesystem::path serialisedModelOutputPath{ std::filesystem::path(NEKI_SOURCE_DIR) / std::string("Samples/Resource-Files/nkmodels/DamagedHelmet/DamagedHelmet.nkmodel") };
//		NK::ModelLoader::SerialiseNKModel("Samples/Resource-Files/DamagedHelmet/DamagedHelmet.gltf", serialisedModelOutputPath, true, true);
		
		m_helmetEntity = m_reg.Create();
		NK::CModelRenderer& modelRenderer{ m_reg.AddComponent<NK::CModelRenderer>(m_helmetEntity) };
		modelRenderer.modelPath = "Samples/Resource-Files/nkmodels/DamagedHelmet/DamagedHelmet.nkmodel";
		NK::CTransform& transform{ m_reg.AddComponent<NK::CTransform>(m_helmetEntity) };
		transform.SetPosition(glm::vec3(0, 0, -3));
		transform.SetRotation({ glm::radians(70.0f), glm::radians(-30.0f), glm::radians(180.0f) });

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
		NK::CTransform& transform{ m_reg.GetComponent<NK::CTransform>(m_helmetEntity) };
		constexpr float speed{ 50.0f };
		const float rotationAmount{ glm::radians(speed * static_cast<float>(NK::TimeManager::GetDeltaTime())) };
		transform.SetRotation(transform.GetRotation() + glm::vec3(0, rotationAmount, 0));
	}


private:
	NK::Entity m_skyboxEntity;
	NK::Entity m_helmetEntity;
	NK::Entity m_cameraEntity;
	NK::UniquePtr<NK::PlayerCamera> m_playerCamera;
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
		m_playerCameraLayer = NK::UniquePtr<NK::PlayerCameraLayer>(NK_NEW(NK::PlayerCameraLayer, m_scenes[m_activeScene]->m_reg));

		m_preAppLayers.push_back(m_windowLayer.get());
		m_preAppLayers.push_back(m_inputLayer.get());
		m_preAppLayers.push_back(m_playerCameraLayer.get());
		
		
		//Post-app layers
		m_modelVisibilityLayer = NK::UniquePtr<NK::ModelVisibilityLayer>(NK_NEW(NK::ModelVisibilityLayer, m_scenes[m_activeScene]->m_reg));
		NK::RenderLayerDesc renderLayerDesc{};
		renderLayerDesc.backend = NK::GRAPHICS_BACKEND::VULKAN;
		renderLayerDesc.enableSSAA = true;
		renderLayerDesc.ssaaMultiplier = 4;
		renderLayerDesc.window = m_window.get();
		m_renderLayer = NK::UniquePtr<NK::RenderLayer>(NK_NEW(NK::RenderLayer, m_scenes[m_activeScene]->m_reg,  renderLayerDesc));

		m_postAppLayers.push_back(m_modelVisibilityLayer.get());
		m_postAppLayers.push_back(m_renderLayer.get());
	}



	virtual void Update() override
	{
		m_scenes[m_activeScene]->Update();

		glfwSetWindowShouldClose(m_window->GetGLFWWindow(), NK::InputManager::GetKeyPressed(NK::KEYBOARD::ESCAPE));
		
		m_shutdown = m_window->ShouldClose();
	}


private:
	NK::UniquePtr<NK::Window> m_window;
	NK::Entity m_windowEntity;

	//Pre-app layers
	NK::UniquePtr<NK::WindowLayer> m_windowLayer;
	NK::UniquePtr<NK::InputLayer> m_inputLayer;
	NK::UniquePtr<NK::PlayerCameraLayer> m_playerCameraLayer;

	//Post-app layers
	NK::UniquePtr<NK::ModelVisibilityLayer> m_modelVisibilityLayer;
	NK::UniquePtr<NK::RenderLayer> m_renderLayer;
};



[[nodiscard]] NK::ContextConfig CreateContext()
{
	NK::LoggerConfig loggerConfig{ NK::LOGGER_TYPE::CONSOLE, false };
	loggerConfig.SetLayerChannelBitfield(NK::LOGGER_LAYER::VULKAN_GENERAL, NK::LOGGER_CHANNEL::WARNING | NK::LOGGER_CHANNEL::ERROR);
	loggerConfig.SetLayerChannelBitfield(NK::LOGGER_LAYER::VULKAN_VALIDATION, NK::LOGGER_CHANNEL::WARNING | NK::LOGGER_CHANNEL::ERROR);
	loggerConfig.SetLayerChannelBitfield(NK::LOGGER_LAYER::TRACKING_ALLOCATOR, NK::LOGGER_CHANNEL::WARNING | NK::LOGGER_CHANNEL::ERROR);

	constexpr NK::TrackingAllocatorConfig trackingAllocatorConfig{ NK::TRACKING_ALLOCATOR_VERBOSITY_FLAGS::ALL };
	constexpr NK::AllocatorConfig allocatorConfig{ NK::ALLOCATOR_TYPE::TRACKING, trackingAllocatorConfig };

	return NK::ContextConfig(loggerConfig, allocatorConfig);
}



[[nodiscard]] NK::EngineConfig CreateEngine()
{
	return NK::EngineConfig(NK_NEW(GameApp));
}