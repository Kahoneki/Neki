#include <filesystem>
#include <Components/CCamera.h>
#include <Components/CInput.h>
#include <Components/CModelRenderer.h>
#include <Components/CSkybox.h>
#include <Components/CTransform.h>
#include <Core/EngineConfig.h>
#include <Core/RAIIContext.h>
#include <Core/Layers/ModelVisibilityLayer.h>
#include <Core/Layers/InputLayer.h>
#include <Core/Layers/PlayerCameraLayer.h>
#include <Core/Layers/RenderLayer.h>
#include <Graphics/Camera/PlayerCamera.h>
#include <Managers/InputManager.h>
#include <Managers/TimeManager.h>

#include "Core/Layers/ServerNetworkLayer.h"


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
	explicit GameApp()
	{
		m_scenes.push_back(NK::UniquePtr<NK::Scene>(NK_NEW(GameScene)));
		m_activeScene = 0;

		NK::ServerNetworkLayerDesc serverNetworkLayerDesc{};
		serverNetworkLayerDesc.maxClients = 1;
		serverNetworkLayerDesc.type = NK::SERVER_TYPE::LAN;
		m_postAppLayers.push_back(NK::UniquePtr<NK::ILayer>(NK_NEW(NK::ServerNetworkLayer, serverNetworkLayerDesc)));
		
		NK::RenderLayerDesc renderLayerDesc{};
		renderLayerDesc.backend = NK::GRAPHICS_BACKEND::NONE;
		m_postAppLayers.push_back(NK::UniquePtr<NK::ILayer>(NK_NEW(NK::RenderLayer, renderLayerDesc)));
		
		const NK::Window* const window{ dynamic_cast<NK::RenderLayer*>(m_postAppLayers[1].get())->GetWindow() };
		
		NK::InputLayerDesc inputLayerDesc{};
		inputLayerDesc.window = window;
		m_preAppLayers.push_back(NK::UniquePtr<NK::ILayer>(NK_NEW(NK::InputLayer, inputLayerDesc)));
		
		m_preAppLayers.push_back(NK::UniquePtr<NK::ILayer>(NK_NEW(NK::PlayerCameraLayer)));

		NK::InputManager::SetWindow(window);
		glfwSetInputMode(window->GetGLFWWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}



	virtual void Update() override
	{
		NK::InputManager::UpdateMouse();
		m_scenes[m_activeScene]->Update();

		const NK::Window* const window{ dynamic_cast<NK::RenderLayer*>(m_postAppLayers[1].get())->GetWindow() };
		glfwSetWindowShouldClose(window->GetGLFWWindow(), NK::InputManager::GetKeyPressed(NK::KEYBOARD::ESCAPE));
		
		m_shutdown = window->ShouldClose();
	}
	
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