#include "CPlayer.h"
#include "PlayerLayer.h"

#include <filesystem>
#include <Components/CCamera.h>
#include <Components/CInput.h>
#include <Components/CModelRenderer.h>
#include <Components/CSkybox.h>
#include <Components/CTransform.h>
#include <Core/EngineConfig.h>
#include <Core/RAIIContext.h>
#include <Core/Layers/ClientNetworkLayer.h>
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
	explicit GameScene() : Scene(2), m_camera(NK::UniquePtr<NK::Camera>(NK_NEW(NK::Camera, glm::vec3(0, 0, -3), 90.0f, 0, 0.01f, 100.0f, 90.0f, WIN_ASPECT_RATIO)))
	{
		
		//ONLY NEEDS TO BE CALLED ONCE - serialises the model into a persistent .nkmodel file that can then be loaded
		//		std::filesystem::path serialisedModelOutputPath{ std::filesystem::path(NEKI_SOURCE_DIR) / std::string("Samples/Resource-Files/nkmodels/DamagedHelmet/DamagedHelmet.nkmodel") };
		//		NK::ModelLoader::SerialiseNKModel("Samples/Resource-Files/DamagedHelmet/DamagedHelmet.gltf", serialisedModelOutputPath, true, true);
		
		m_playerEntity = m_reg.Create();
		NK::CModelRenderer& modelRenderer{ m_reg.AddComponent<NK::CModelRenderer>(m_playerEntity) };
		modelRenderer.modelPath = "Samples/Resource-Files/nkmodels/DamagedHelmet/DamagedHelmet.nkmodel";
		NK::CTransform& transform{ m_reg.AddComponent<NK::CTransform>(m_playerEntity) };
		transform.SetPosition(glm::vec3(0, 0, 3));
		transform.SetRotation({ glm::radians(70.0f), glm::radians(150.0f), glm::radians(180.0f) });
		CPlayer& player{ m_reg.AddComponent<CPlayer>(m_playerEntity) };
		player.movementSpeed = 10.0f;

		m_cameraEntity = m_reg.Create();
		NK::CCamera& camera{ m_reg.AddComponent<NK::CCamera>(m_cameraEntity) };
		camera.camera = m_camera.get();


		//Inputs
		NK::ButtonBinding aBinding{ NK::KEYBOARD::A };
		NK::ButtonBinding dBinding{ NK::KEYBOARD::D };
		NK::ButtonBinding sBinding{ NK::KEYBOARD::S };
		NK::ButtonBinding wBinding{ NK::KEYBOARD::W };
		NK::Axis1DBinding moveHorizontalBinding{ { aBinding, dBinding }, { -1, 1 } };
		NK::Axis1DBinding moveVerticalBinding{ { sBinding, wBinding }, { -1, 1 } };
		NK::Axis2DBinding moveBinding{ NK::Axis2DBinding({ moveHorizontalBinding, moveVerticalBinding }) };
		NK::InputManager::BindActionToInput(PLAYER_ACTIONS::MOVE, moveBinding);
		
		NK::CInput& input{ m_reg.AddComponent<NK::CInput>(m_playerEntity) };
		input.AddActionToMap(PLAYER_ACTIONS::MOVE);
	}

	
	virtual void Update() override
	{
	}


private:
	NK::Entity m_playerEntity;
	NK::Entity m_cameraEntity;
	NK::UniquePtr<NK::Camera> m_camera;
};


class GameApp final : public NK::Application
{
public:
	explicit GameApp() : Application(1)
	{
		//Register types
		NK::TypeRegistry::Register<PLAYER_ACTIONS>("PLAYER_ACTIONS");
		
		
		m_scenes.push_back(NK::UniquePtr<NK::Scene>(NK_NEW(GameScene)));
		m_activeScene = 0;


		//Window
		NK::WindowDesc windowDesc;
		windowDesc.name = "Networking Sample";
		windowDesc.size = { 400, 400 };
		m_window = NK::UniquePtr<NK::Window>(NK_NEW(NK::Window, windowDesc));

		m_windowEntity = m_reg.Create();
		NK::CWindow& windowComponent{ m_reg.AddComponent<NK::CWindow>(m_windowEntity) };
		windowComponent.window = m_window.get();
		

		//Pre-app layers
		m_windowLayer = NK::UniquePtr<NK::WindowLayer>(NK_NEW(NK::WindowLayer, m_reg));
		NK::InputLayerDesc inputLayerDesc{ m_window.get() };
		m_inputLayer = NK::UniquePtr<NK::InputLayer>(NK_NEW(NK::InputLayer, m_scenes[m_activeScene]->m_reg, inputLayerDesc));
		m_playerLayer = NK::UniquePtr<PlayerLayer>(NK_NEW(PlayerLayer, m_scenes[m_activeScene]->m_reg));
		NK::ClientNetworkLayerDesc clientDesc{};
		m_clientNetworkLayer = NK::UniquePtr<NK::ClientNetworkLayer>(NK_NEW(NK::ClientNetworkLayer, m_scenes[m_activeScene]->m_reg, clientDesc));
		
		m_preAppLayers.push_back(m_windowLayer.get());
		m_preAppLayers.push_back(m_inputLayer.get());
		m_preAppLayers.push_back(m_playerLayer.get());
		m_preAppLayers.push_back(m_clientNetworkLayer.get());
		
		
		//Post-app layers
		NK::RenderLayerDesc renderLayerDesc{};
		renderLayerDesc.backend = NK::GRAPHICS_BACKEND::VULKAN;
		renderLayerDesc.enableMSAA = true;
		renderLayerDesc.msaaSampleCount = NK::SAMPLE_COUNT::BIT_8;
		renderLayerDesc.window = m_window.get();
		m_renderLayer = NK::UniquePtr<NK::RenderLayer>(NK_NEW(NK::RenderLayer, m_scenes[m_activeScene]->m_reg,  renderLayerDesc));

		m_postAppLayers.push_back(m_renderLayer.get());
		m_postAppLayers.push_back(m_clientNetworkLayer.get());
		

		const NK::NETWORK_LAYER_ERROR_CODE err{ m_clientNetworkLayer->Connect("192.168.0.202", 7777) };
		if (!NET_SUCCESS(err))
		{
			NK::Context::GetLogger()->IndentLog(NK::LOGGER_CHANNEL::ERROR, NK::LOGGER_LAYER::APPLICATION, "Failed to connect - error = " + std::to_string(std::to_underlying(err)) + "\n");
			throw std::runtime_error("");
		}
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
	NK::UniquePtr<PlayerLayer> m_playerLayer;
	
	//Post-app layers
	NK::UniquePtr<NK::RenderLayer> m_renderLayer;
	NK::UniquePtr<NK::ClientNetworkLayer> m_clientNetworkLayer;
};



[[nodiscard]] NK::ContextConfig CreateContext()
{
	NK::LoggerConfig loggerConfig{ NK::LOGGER_TYPE::CONSOLE, true };
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