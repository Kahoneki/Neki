#include "PlayerCamera.h"

#include <Managers/InputManager.h>
#include <Managers/TimeManager.h>


namespace NK
{
	PlayerCamera::PlayerCamera(glm::vec3 _pos, float _yaw, float _pitch, float _nearPlaneDist, float _farPlaneDist, float _fov, float _aspectRatio, float _movementSpeed, float _mouseSensitivity)
	: Camera(_pos, _yaw, _pitch, _nearPlaneDist, _farPlaneDist, _fov, _aspectRatio), m_movementSpeed(_movementSpeed), m_mouseSensitivity(_mouseSensitivity)
	{}


	
	void PlayerCamera::Update()
	{
		//Process keyboard input
		const float speed{ m_movementSpeed * static_cast<float>(TimeManager::GetDeltaTime()) };
		glm::vec3 movementDirection{};
		if (std::get<KEY_INPUT_STATE>(InputManager::GetInputState(INPUT_ACTION::MOVE_FORWARDS))		== KEY_INPUT_STATE::HELD) { movementDirection += m_forward; }
		if (std::get<KEY_INPUT_STATE>(InputManager::GetInputState(INPUT_ACTION::MOVE_BACKWARDS))	== KEY_INPUT_STATE::HELD) { movementDirection -= m_forward; }
		if (std::get<KEY_INPUT_STATE>(InputManager::GetInputState(INPUT_ACTION::MOVE_LEFT))			== KEY_INPUT_STATE::HELD) { movementDirection -= m_right; }
		if (std::get<KEY_INPUT_STATE>(InputManager::GetInputState(INPUT_ACTION::MOVE_RIGHT))		== KEY_INPUT_STATE::HELD) { movementDirection += m_right; }
		if (movementDirection != glm::vec3(0))
		{
			m_pos += glm::normalize(movementDirection) * speed;
		}
		
		//Process mouse movement
		const float oldMouseX{ static_cast<float>(std::get<MOUSE_INPUT_STATE>(InputManager::GetInputStateLastUpdate(INPUT_ACTION::CAMERA_YAW))) };
		const float oldMouseY{ static_cast<float>(std::get<MOUSE_INPUT_STATE>(InputManager::GetInputStateLastUpdate(INPUT_ACTION::CAMERA_PITCH))) };
		const float newMouseX{ static_cast<float>(std::get<MOUSE_INPUT_STATE>(InputManager::GetInputState(INPUT_ACTION::CAMERA_YAW))) };
		const float newMouseY{ static_cast<float>(std::get<MOUSE_INPUT_STATE>(InputManager::GetInputState(INPUT_ACTION::CAMERA_PITCH))) };
		const float yawOffset{ (newMouseX - oldMouseX) * m_mouseSensitivity };
		const float pitchOffset{ (newMouseY - oldMouseY) * m_mouseSensitivity };
		m_pitch -= pitchOffset;
		m_yaw -= yawOffset;

		//Constrain pitch for first person camera
		if		(m_pitch > 89.0f) { m_pitch = 89.0f; }
		else if (m_pitch < -89.0f) { m_pitch = -89.0f; }

		m_viewMatDirty = true;
		m_perspectiveProjMatDirty = true;
		m_orthographicProjMatDirty = true;
		
		UpdateCameraVectors();
	}
	
}