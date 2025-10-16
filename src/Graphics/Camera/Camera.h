#pragma once

#include <Types/NekiTypes.h>

#include <glm/glm.hpp>


namespace NK
{

	class Camera
	{
	public:
		//Neki is built on a left handed coordinate system (+X is right, +Y is up, +Z is forward)
		//A yaw of 0 is equivalent to looking at the +X axis, increasing yaw will rotate the camera around the Y axis in an anti-clockwise direction measured in degrees
		//E.g.: setting the yaw to +90 will have the camera look along +Z, +-180 will be -X, and -90 will be -Z
		explicit Camera(glm::vec3 _pos, float _yaw, float _pitch, float _nearPlaneDist, float _farPlaneDist, float _fov, float _aspectRatio);
		~Camera() = default;
		
		//Matrices are cached and only updated when necessary - virtually no overhead in calling this function
		[[nodiscard]] CameraShaderData GetCameraShaderData(const PROJECTION_METHOD _method);
		
		[[nodiscard]] inline glm::vec3 GetPosition() const { return m_pos; }
		[[nodiscard]] inline float GetYaw() const { return m_yaw; }
		[[nodiscard]] inline float GetPitch() const { return m_pitch; }
		[[nodiscard]] inline float GetNearPlaneDistance() const { return m_nearPlaneDist; }
		[[nodiscard]] inline float GetFarPlaneDistance() const { return m_farPlaneDist; }
		[[nodiscard]] inline float GetFOV() const { return m_fov; }
		[[nodiscard]] inline float GetAspectRatio() const { return m_aspectRatio; }

		//View-matrix-dirtying setters
		inline void SetPosition(const glm::vec3 _value)	{ m_pos = _value; m_viewMatDirty = true; }
		inline void SetYaw(const float _value) { m_yaw = _value; m_viewMatDirty = true; }
		inline void SetPitch(const float _value) { m_pitch = _value; m_viewMatDirty = true; }
		
		//Projection-matrices-dirtying setters
		inline void SetNearPlaneDistance(const float _value) { m_nearPlaneDist = _value; m_orthographicProjMatDirty = true; m_perspectiveProjMatDirty = true; }
		inline void SetFarPlaneDistance(const float _value)	{ m_farPlaneDist = _value; m_orthographicProjMatDirty = true; m_perspectiveProjMatDirty = true; }
		inline void SetFOV(const float _value) { m_fov = _value; m_orthographicProjMatDirty = true; m_perspectiveProjMatDirty = true; }
		inline void SetAspectRatio(const float _value) { m_aspectRatio = _value; m_orthographicProjMatDirty = true; m_perspectiveProjMatDirty = true; }
		


	protected:
		//Will update matrices if they're dirty
		[[nodiscard]] glm::mat4 GetViewMatrix();
		[[nodiscard]] glm::mat4 GetProjectionMatrix(const PROJECTION_METHOD _method);
		
		//Updates m_up, m_forward, and m_right based on m_yaw and m_pitch
		void UpdateCameraVectors();
		
		
		glm::vec3 m_pos;
		glm::vec3 m_up;
		glm::vec3 m_forward;
		glm::vec3 m_right;
		float m_yaw;
		float m_pitch;
		float m_nearPlaneDist;
		float m_farPlaneDist;
		float m_fov;
		float m_aspectRatio;

		glm::mat4 m_viewMat;
		glm::mat4 m_orthographicProjMat;
		glm::mat4 m_perspectiveProjMat;
		bool m_viewMatDirty; //True if m_pos, m_yaw, or m_pitch has been changed
		//Need two flags even though they're based on the same state so we don't have to unnecessarily recalculate, for example, orthographic matrix if user is only requesting perspective matrix
		bool m_orthographicProjMatDirty; //True if m_fov, m_aspectRatio, m_nearPlaneDist, or m_farPlaneDist has been changed
		bool m_perspectiveProjMatDirty; //True if m_fov, m_aspectRatio, m_nearPlaneDist, or m_farPlaneDist has been changed
	};
	
}