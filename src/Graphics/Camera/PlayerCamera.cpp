#include "PlayerCamera.h"

#include <Managers/InputManager.h>
#include <Managers/TimeManager.h>


namespace NK
{
	PlayerCamera::PlayerCamera(glm::vec3 _pos, float _yaw, float _pitch, float _nearPlaneDist, float _farPlaneDist, float _fov, float _aspectRatio, float _movementSpeed, float _mouseSensitivity)
	: Camera(_pos, _yaw, _pitch, _nearPlaneDist, _farPlaneDist, _fov, _aspectRatio), m_movementSpeed(_movementSpeed), m_mouseSensitivity(_mouseSensitivity)
	{}
	
}