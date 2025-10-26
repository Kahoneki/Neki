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
	explicit GameScene() : Scene(0)
	{

	}

	
	virtual void Update() override
	{
		
	}


private:
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
		windowDesc.name = "Networking Sample";
		windowDesc.size = { 1920, 1080 };
		m_window = NK::UniquePtr<NK::Window>(NK_NEW(NK::Window, windowDesc));

		m_windowEntity = m_reg.Create();
		NK::CWindow& windowComponent{ m_reg.AddComponent<NK::CWindow>(m_windowEntity) };
		windowComponent.window = m_window.get();
		

		//Pre-app layers
		m_windowLayer = NK::UniquePtr<NK::WindowLayer>(NK_NEW(NK::WindowLayer, m_reg));
		NK::InputLayerDesc inputLayerDesc{ m_window.get() };
		m_inputLayer = NK::UniquePtr<NK::InputLayer>(NK_NEW(NK::InputLayer, m_scenes[m_activeScene]->m_reg, inputLayerDesc));
		
		m_preAppLayers.push_back(m_windowLayer.get());
		m_preAppLayers.push_back(m_inputLayer.get());
		
		
		//Post-app layers
		NK::ClientNetworkLayerDesc clientDesc{};
		m_clientNetworkLayer = NK::UniquePtr<NK::ClientNetworkLayer>(NK_NEW(NK::ClientNetworkLayer, m_reg, clientDesc));
		
		m_postAppLayers.push_back(m_clientNetworkLayer.get());
		

		const NK::NETWORK_LAYER_ERROR_CODE err{ m_clientNetworkLayer->Connect("192.168.0.202", 7777) };
		if (!NET_SUCCESS(err))
		{
			NK::Context::GetLogger()->IndentLog(NK::LOGGER_CHANNEL::ERROR, NK::LOGGER_LAYER::APPLICATION, "Failed to connect - error = " + std::to_string(std::to_underlying(err)) + "\n");
			throw std::runtime_error("");
		}
//		m_clientNetworkLayer->Disconnect();
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
	
	//Post-app layers
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