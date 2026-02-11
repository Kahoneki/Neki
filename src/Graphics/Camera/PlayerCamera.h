#pragma once

#include "Camera.h"

#include <glm/glm.hpp>


namespace NK
{

	class PlayerCamera final : public Camera
	{
		friend class PlayerCameraLayer;
		friend class cereal::access;
		
		
	public:
		explicit PlayerCamera(float _nearPlaneDist, float _farPlaneDist, float _fov, float _aspectRatio, float _movementSpeed, float _mouseSensitivity);
		virtual ~PlayerCamera() override = default;

		[[nodiscard]] inline bool GetFlightEnabled() const { return m_flightEnabled; }
		[[nodiscard]] inline float GetMovementSpeed() const { return m_movementSpeed; }
		[[nodiscard]] inline float GetMouseSensitivity() const { return m_mouseSensitivity; }

		inline void SetFlightEnabled(const bool _value) { m_flightEnabled = _value; }
		inline void SetMovementSpeed(const float _value) { m_movementSpeed = _value; }
		inline void SetMouseSensitivity(const float _value) { m_mouseSensitivity = _value; }
		
		
		SERIALISE_MEMBER_FUNC(cereal::base_class<Camera>(this), m_movementSpeed, m_mouseSensitivity)
		
	
	private:
		PlayerCamera() = default;
		
		float m_yaw;
		float m_pitch;
		bool m_flightEnabled{ true }; //If true, camera can fly. If false, camera movement is locked on global y
		float m_movementSpeed;
		float m_mouseSensitivity;
		bool m_firstFrame{ true }; //first frame the component is active
	};

}