#pragma once

#include "Camera.h"

#include <glm/glm.hpp>


namespace NK
{

	class PlayerCamera final : public Camera
	{
	public:
		//Neki is built on a left handed coordinate system (+X is right, +Y is up, +Z is forward)
		//A yaw of 0 is equivalent to looking at the +X axis, increasing yaw will rotate the camera around the Y axis in an anti-clockwise direction measured in degrees
		//E.g.: setting the yaw to +90 will have the camera look along +Z, +-180 will be -X, and -90 will be -Z
		explicit PlayerCamera(glm::vec3 _pos, float _yaw, float _pitch, float _nearPlaneDist, float _farPlaneDist, float _fov, float _aspectRatio, float _movementSpeed, float _mouseSensitivity);
		~PlayerCamera() = default;

		void Update();

		[[nodiscard]] inline float GetMovementSpeed() const { return m_movementSpeed; }
		[[nodiscard]] inline float GetMouseSensitivity() const { return m_mouseSensitivity; }
		
		inline void SetMovementSpeed(const float _value) { m_movementSpeed = _value; }
		inline void SetMouseSensitivity(const float _value) { m_mouseSensitivity = _value; }
		
	
	protected:
		float m_movementSpeed;
		float m_mouseSensitivity;
	};

}