#include "PlayerCameraLayer.h"

#include <Components/CCamera.h>
#include <Components/CInput.h>
#include <Components/CLight.h>
#include <Graphics/Camera/PlayerCamera.h>
#include <Managers/InputManager.h>
#include <Managers/TimeManager.h>

#include <glm/gtx/norm.hpp>


namespace NK
{

	PlayerCameraLayer::PlayerCameraLayer(Registry& _reg) : ILayer(_reg)
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



	void PlayerCameraLayer::Update()
	{
		if (Context::GetEditorActive())
		{
			return;
		}
		
		if (InputManager::GetActionInputType(PLAYER_CAMERA_ACTIONS::MOVE) != INPUT_BINDING_TYPE::AXIS_2D)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::PLAYER_CAMERA_LAYER, "PlayerCameraLayer::Update() - PLAYER_CAMERA_ACTIONS::MOVE is not bound to INPUT_TYPE::AXIS_2D as required");
		}

		if (InputManager::GetActionInputType(PLAYER_CAMERA_ACTIONS::YAW_PITCH) != INPUT_BINDING_TYPE::AXIS_2D)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::PLAYER_CAMERA_LAYER, "PlayerCameraLayer::Update() - PLAYER_CAMERA_ACTIONS::YAW_PITCH is not bound to INPUT_TYPE::AXIS_2D as required");
		}

		
		CLight* debugLight{ Context::GetActiveLightView() };
		if (debugLight)
		{
			if (debugLight->GetLightType() == LIGHT_TYPE::POINT)
			{
				CTransform& transform{ m_reg.get().GetComponent<CTransform>(m_reg.get().GetEntity(*debugLight)) };
				const Axis2DState mouseState{ InputManager::GetAxis2DState(PLAYER_CAMERA_ACTIONS::YAW_PITCH) };
				
				if (mouseState.values.x != 0.0f || mouseState.values.y != 0.0f || debugLight->firstFrame)
				{
					constexpr float sensitivity{ 0.05f };
				
					const float pitchDelta{ glm::radians(mouseState.values.y * sensitivity) };
					const float yawDelta{ glm::radians(mouseState.values.x * sensitivity) };
            
					debugLight->pitch += pitchDelta;
					debugLight->yaw += yawDelta;

					//Clamp pitch
					constexpr float pitchLimit = glm::radians(89.0f);
					debugLight->pitch = std::clamp(debugLight->pitch, -pitchLimit, pitchLimit);

					//one fresh quaternion coming right up! (hold the euler extraction!)
					glm::quat pitchQuat = glm::angleAxis(debugLight->pitch, glm::vec3(1.0f, 0.0f, 0.0f));
					glm::quat yawQuat = glm::angleAxis(debugLight->yaw, glm::vec3(0.0f, 1.0f, 0.0f));
					transform.SetLocalRotation(yawQuat * pitchQuat);
				
					debugLight->firstFrame = false;
				}
			}
			
			//Don't process regular camera movement if debug light is active
			return;
		}
		
		
		for (auto&& [camera, input, transform] : m_reg.get().View<CCamera, CInput, CTransform>())
		{
			PlayerCamera* pc{ dynamic_cast<PlayerCamera*>(camera.camera.get()) };
			if (!pc) { continue; }
			
			//Rotation
			const Axis2DState mouseState{ input.GetActionState<Axis2DState>(PLAYER_CAMERA_ACTIONS::YAW_PITCH) };
			if (mouseState.values.x != 0.0f || mouseState.values.y != 0.0f || pc->m_firstFrame)
			{
				const float pitchDelta{ glm::radians(mouseState.values.y * pc->GetMouseSensitivity()) };
				const float yawDelta{ glm::radians(mouseState.values.x * pc->GetMouseSensitivity()) };
            
				pc->m_pitch += pitchDelta;
				pc->m_yaw += yawDelta;

				//Clamp pitch
				constexpr float pitchLimit = glm::radians(89.0f);
				pc->m_pitch = std::clamp(pc->m_pitch, -pitchLimit, pitchLimit);

				//one fresh quaternion coming right up! (hold the euler extraction!)
				glm::quat pitchQuat = glm::angleAxis(pc->m_pitch, glm::vec3(1.0f, 0.0f, 0.0f));
				glm::quat yawQuat = glm::angleAxis(pc->m_yaw, glm::vec3(0.0f, 1.0f, 0.0f));
				transform.SetLocalRotation(yawQuat * pitchQuat);
				
				pc->m_firstFrame = false;
			}


			//Movement
			const Axis2DState moveState{ input.GetActionState<Axis2DState>(PLAYER_CAMERA_ACTIONS::MOVE) };
			if (moveState.values.x != 0.0f || moveState.values.y != 0.0f)
			{
				const glm::quat orientation{ transform.GetLocalRotationQuat() };
				glm::vec3 forward{ orientation * glm::vec3(0.0f, 0.0f, 1.0f) };
				if (!pc->GetFlightEnabled()) { forward = glm::normalize(glm::vec3(forward.x, 0.0f, forward.z)); }
				constexpr glm::vec3 worldUp{ 0.0f, 1.0f, 0.0f };
				const glm::vec3 globalRight{ glm::normalize(glm::cross(worldUp, forward)) };

				glm::vec3 movementDirection{ 0.0f };
				movementDirection += globalRight * moveState.values.x;
				movementDirection += forward * moveState.values.y;

				if (glm::length2(movementDirection) > 0.0001f)
				{
					transform.SetLocalPosition(transform.GetLocalPosition() + glm::normalize(movementDirection) * pc->GetMovementSpeed() * static_cast<float>(TimeManager::GetDeltaTime()));
				}
			}
		}
	}

}
