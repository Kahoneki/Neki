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
#include <Core/Layers/InputLayer.h>
#include <Core/Layers/ModelVisibilityLayer.h>
#include <Core/Layers/PlayerCameraLayer.h>
#include <Core/Layers/RenderLayer.h>
#include <Core/Layers/ServerNetworkLayer.h>
#include <Core/Layers/WindowLayer.h>
#include <Graphics/Camera/PlayerCamera.h>
#include <Managers/InputManager.h>
#include <Managers/TimeManager.h>


class GameScene final : public NK::Scene
{
public:
	explicit GameScene() : Scene(1)
	{
		m_helmetEntity = m_reg.Create();
		NK::CInput& input{ m_reg.AddComponent<NK::CInput>(m_helmetEntity) };
		NK::CTransform& transform{ m_reg.AddComponent<NK::CTransform>(m_helmetEntity) };
		transform.SetPosition(glm::vec3(0, 0, 3));
		transform.SetRotation({ glm::radians(70.0f), glm::radians(150.0f), glm::radians(180.0f) });
		CPlayer& player{ m_reg.AddComponent<CPlayer>(m_helmetEntity) };
		player.movementSpeed = 10.0f;

		NK::ButtonBinding aBinding{ NK::KEYBOARD::A };
		NK::ButtonBinding dBinding{ NK::KEYBOARD::D };
		NK::ButtonBinding sBinding{ NK::KEYBOARD::S };
		NK::ButtonBinding wBinding{ NK::KEYBOARD::W };
		NK::Axis1DBinding moveHorizontalBinding{ { aBinding, dBinding }, { -1, 1 } };
		NK::Axis1DBinding moveVerticalBinding{ { sBinding, wBinding }, { -1, 1 } };
		NK::Axis2DBinding moveBinding{ NK::Axis2DBinding({ moveHorizontalBinding, moveVerticalBinding }) };
		NK::InputManager::BindActionToInput(PLAYER_ACTIONS::MOVE, moveBinding);
//		input.AddActionToMap(PLAYER_ACTIONS::MOVE);
	}

	
	virtual void Update() override
	{
//		if (m_reg.GetComponent<NK::CInput>(m_playerEntity).actionStates.empty()) { return; }
//		const NK::Axis2DState state{ m_reg.GetComponent<NK::CInput>(m_playerEntity).GetActionState<NK::Axis2DState>(PLAYER_ACTIONS::MOVE) };
//		NK::Context::GetLogger()->IndentLog(NK::LOGGER_CHANNEL::SUCCESS, NK::LOGGER_LAYER::APPLICATION, "Move state: " + std::to_string(state.values.x) + ", " + std::to_string(state.values.y) + "\n");
	}


private:
	NK::Entity m_helmetEntity;
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
		NK::InputManager::SetWindow(m_window.get());

		m_windowEntity = m_reg.Create();
		NK::CWindow& windowComponent{ m_reg.AddComponent<NK::CWindow>(m_windowEntity) };
		windowComponent.window = m_window.get();
		

		//Pre-app layers
		m_windowLayer = NK::UniquePtr<NK::WindowLayer>(NK_NEW(NK::WindowLayer, m_reg));
		NK::ServerNetworkLayerDesc serverDesc{};
		serverDesc.maxClients = 2;
		serverDesc.type = NK::SERVER_TYPE::LAN;
		m_serverNetworkLayer = NK::UniquePtr<NK::ServerNetworkLayer>(NK_NEW(NK::ServerNetworkLayer, m_scenes[m_activeScene]->m_reg, serverDesc));
		m_playerLayer = NK::UniquePtr<PlayerLayer>(NK_NEW(PlayerLayer, m_scenes[m_activeScene]->m_reg));
		
		m_preAppLayers.push_back(m_windowLayer.get());
		m_preAppLayers.push_back(m_serverNetworkLayer.get());
		
		
		//Post-app layers
		m_postAppLayers.push_back(m_playerLayer.get());
		m_postAppLayers.push_back(m_serverNetworkLayer.get());

		
		const NK::NETWORK_LAYER_ERROR_CODE err{ m_serverNetworkLayer->Host(7777) };
		if (!NET_SUCCESS(err))
		{
			NK::Context::GetLogger()->IndentLog(NK::LOGGER_CHANNEL::ERROR, NK::LOGGER_LAYER::APPLICATION, "Failed to host - error = " + std::to_string(std::to_underlying(err)) + "\n");
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
	NK::UniquePtr<NK::ServerNetworkLayer> m_serverNetworkLayer;
	NK::UniquePtr<PlayerLayer> m_playerLayer;
	
	//Post-app layers
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