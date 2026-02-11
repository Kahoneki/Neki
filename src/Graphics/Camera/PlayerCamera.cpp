#include "PlayerCamera.h"


namespace NK
{
	PlayerCamera::PlayerCamera(float _nearPlaneDist, float _farPlaneDist, float _fov, float _aspectRatio, float _movementSpeed, float _mouseSensitivity)
	: Camera(_nearPlaneDist, _farPlaneDist, _fov, _aspectRatio), m_yaw(0.0f), m_pitch(0.0f), m_movementSpeed(_movementSpeed), m_mouseSensitivity(_mouseSensitivity)
	{}
	
}