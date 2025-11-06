#include "PlayerLayer.h"

#include <Components/CCamera.h>
#include <Components/CInput.h>
#include <Components/CTransform.h>
#include <Graphics/Camera/PlayerCamera.h>
#include <Managers/InputManager.h>
#include <Managers/TimeManager.h>


PlayerLayer::PlayerLayer(NK::Registry& _reg) : ILayer(_reg)
{
	m_logger.Indent();
	m_logger.Log(NK::LOGGER_CHANNEL::HEADING, NK::LOGGER_LAYER::APPLICATION, "Initialising Player Layer\n");

	m_playerJumpEventSubscriptionID = NK::EventManager::Subscribe<PlayerLayer, PlayerJumpEvent>(this, &PlayerLayer::HandleJump);

	m_logger.Unindent();
}



PlayerLayer::~PlayerLayer()
{
	m_logger.Indent();
	m_logger.Log(NK::LOGGER_CHANNEL::HEADING, NK::LOGGER_LAYER::APPLICATION, "Shutting Down Player Layer\n");

	NK::EventManager::Unsubscribe<PlayerJumpEvent>(m_playerJumpEventSubscriptionID);

	m_logger.Unindent();
}



void PlayerLayer::Update()
{
	if (NK::InputManager::GetActionInputType(PLAYER_ACTIONS::MOVE) != NK::INPUT_BINDING_TYPE::AXIS_2D)
	{
		m_logger.IndentLog(NK::LOGGER_CHANNEL::ERROR, NK::LOGGER_LAYER::APPLICATION, "PlayerLayer::Update() - PLAYER_ACTIONS::MOVE is not bound to NK::INPUT_TYPE::AXIS_2D as required\n");
	}

	if (NK::InputManager::GetActionInputType(PLAYER_ACTIONS::JUMP) != NK::INPUT_BINDING_TYPE::BUTTON)
	{
		m_logger.IndentLog(NK::LOGGER_CHANNEL::ERROR, NK::LOGGER_LAYER::APPLICATION, "PlayerLayer::Update() - PLAYER_ACTIONS::JUMP is not bound to NK::INPUT_TYPE::BUTTON as required\n");
	}

	for (auto&& [player, transform, input] : m_reg.get().View<CPlayer, NK::CTransform, NK::CInput>())
	{
		//Process movement
		const NK::Axis2DState moveState{ input.GetActionState<NK::Axis2DState>(PLAYER_ACTIONS::MOVE) };
		glm::vec3 movementDirection{ moveState.values.x, 0, moveState.values.y };
		if (movementDirection != glm::vec3(0))
		{
			transform.SetPosition(transform.GetPosition() + glm::normalize(movementDirection) * player.movementSpeed * static_cast<float>(NK::TimeManager::GetDeltaTime()));
		}


		//Process jumping
		const NK::ButtonState jumpState{ input.GetActionState<NK::ButtonState>(PLAYER_ACTIONS::JUMP) };
		if (jumpState.held)
		{
			
		}
	}
}



void PlayerLayer::HandleJump(const PlayerJumpEvent& _event)
{
	NK::CTransform& transform{ m_reg.get().GetComponent<NK::CTransform&>(_event.entity) };
	transform.SetPosition(transform.GetPosition() + glm::vec3(0, 0.25f, 0));
}