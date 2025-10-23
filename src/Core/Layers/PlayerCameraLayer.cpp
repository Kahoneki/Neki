#include "PlayerCameraLayer.h"

#include <Components/CCamera.h>
#include <Components/CInput.h>
#include <Graphics/Camera/PlayerCamera.h>
#include <Managers/InputManager.h>
#include <Managers/TimeManager.h>


namespace NK
{

	PlayerCameraLayer::PlayerCameraLayer()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::PLAYER_CAMERA_LAYER, "Initialising Player Camera Layer\n");

		m_logger.Unindent();
	}



	PlayerCameraLayer::~PlayerCameraLayer()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::PLAYER_CAMERA_LAYER, "Shutting Down Player Camera Layer\n");

		m_logger.Unindent();
	}



	void PlayerCameraLayer::Update(Registry& _reg)
	{
		if (InputManager::GetActionInputType(PLAYER_CAMERA_ACTIONS::MOVE) != INPUT_BINDING_TYPE::AXIS_2D)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::PLAYER_CAMERA_LAYER, "PlayerCameraLayer::Update() - PLAYER_CAMERA_ACTIONS::MOVE is not bound to INPUT_TYPE::AXIS_2D as required");
		}

		if (InputManager::GetActionInputType(PLAYER_CAMERA_ACTIONS::YAW_PITCH) != INPUT_BINDING_TYPE::AXIS_2D)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::PLAYER_CAMERA_LAYER, "PlayerCameraLayer::Update() - PLAYER_CAMERA_ACTIONS::YAW_PITCH is not bound to INPUT_TYPE::AXIS_2D as required");
		}

		for (auto&& [camera, input] : _reg.View<CCamera, CInput>())
		{
			PlayerCamera* pc{ dynamic_cast<PlayerCamera*>(camera.camera) };
			if (!pc) { continue; }
			
			for (std::unordered_map<ActionTypeMapKey, INPUT_STATE_VARIANT>::iterator it{ input.actionStates.begin() }; it != input.actionStates.end(); ++it)
			{

				//Process movement
				const Axis2DState moveState{ input.GetActionState<Axis2DState>(PLAYER_CAMERA_ACTIONS::MOVE) };
				glm::vec3 movementDirection{ 0, 0, 0 };
				movementDirection += pc->m_right * moveState.values.x;
				movementDirection += pc->m_forward * moveState.values.y;
				if (movementDirection != glm::vec3(0))
				{
					pc->m_pos += glm::normalize(movementDirection) * pc->m_movementSpeed * static_cast<float>(TimeManager::GetDeltaTime());
				}

				//Process mouse movement
				const Axis2DState mouseState{ input.GetActionState<Axis2DState>(PLAYER_CAMERA_ACTIONS::YAW_PITCH) }; //Expected to be bound to mouse-diff
				const float pitchOffset{ mouseState.values.y * pc->m_mouseSensitivity };
				const float yawOffset{ mouseState.values.x * pc->m_mouseSensitivity };
				pc->m_pitch -= pitchOffset;
				pc->m_yaw -= yawOffset;

				//Constrain pitch for first person camera
				if		(pc->m_pitch > 89.0f) { pc->m_pitch = 89.0f; }
				else if (pc->m_pitch < -89.0f) { pc->m_pitch = -89.0f; }

				pc->m_viewMatDirty = true;
				pc->m_perspectiveProjMatDirty = true;
				pc->m_orthographicProjMatDirty = true;

				//Calculate new forward vector
				glm::vec3 front;
				front.x = static_cast<float>(cos(glm::radians(pc->m_yaw)) * cos(glm::radians(pc->m_pitch)));
				front.y = static_cast<float>(sin(glm::radians(pc->m_pitch)));
				front.z = static_cast<float>(sin(glm::radians(pc->m_yaw)) * cos(glm::radians(pc->m_pitch)));
				pc->m_forward = glm::normalize(front);

				//Re-calculate right and up vectors
				constexpr glm::vec3 worldUp{ glm::vec3(0, 1, 0) };
				pc->m_right = glm::normalize(glm::cross(worldUp, pc->m_forward));
				pc->m_up = glm::normalize(glm::cross(pc->m_forward, pc->m_right));
			}
		}
	}

}