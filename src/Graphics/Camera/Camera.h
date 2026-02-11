#pragma once
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "ViewFrustum.h"

#include <Core/Context.h>
#include <Types/NekiTypes.h>

#include <glm/glm.hpp>

//Special value that render system will check for each update and set the aspect ratio to the window's aspect ratio
#define WIN_ASPECT_RATIO -2346773 //Random value that hopefully the user wasn't planning on actually using as their aspect ratio


namespace NK
{

	class Camera
	{
		friend class cereal::access;
		
		
	public:
		//Neki is built on a left handed coordinate system (+X is right, +Y is up, +Z is forward)
		//A yaw of 0 is equivalent to looking at the +X axis, increasing yaw will rotate the camera around the Y axis in an anti-clockwise direction measured in degrees
		//E.g.: setting the yaw to +90 will have the camera look along +Z, +-180 will be -X, and -90 will be -Z
		explicit Camera(float _nearPlaneDist, float _farPlaneDist, float _fov, float _aspectRatio);
		Camera() : m_nearPlaneDist(0.01f), m_farPlaneDist(1000.0f), m_fov(90.0f), m_aspectRatio(WIN_ASPECT_RATIO), m_frustum()
		{
			m_orthographicProjMatDirty = true;
			m_perspectiveProjMatDirty = true;
		}

		virtual ~Camera() = default;
		
		//Matrices are cached and only updated when necessary - virtually no overhead in calling this function
		[[nodiscard]] CameraShaderData GetCurrentCameraShaderData(const PROJECTION_METHOD _method, const glm::mat4& _transformationMatrix);
		
		[[nodiscard]] inline float GetNearPlaneDistance() const { return m_nearPlaneDist; }
		[[nodiscard]] inline float GetFarPlaneDistance() const { return m_farPlaneDist; }
		[[nodiscard]] inline float GetFOV() const { return m_fov; }
		[[nodiscard]] inline float GetAspectRatio() const { return m_aspectRatio; }
		[[nodiscard]] inline ViewFrustum GetFrustum()
		{
			//Unused
			// if (m_viewMatDirty || m_perspectiveProjMatDirty)
			// {
			// 	//Frustum is dirty
			// 	m_frustum.Update(GetProjectionMatrix(PROJECTION_METHOD::PERSPECTIVE) * GetViewMatrix());
			// }
			// return m_frustum;
			return ViewFrustum();
		}
		
		//Projection-matrices-dirtying setters
		inline void SetNearPlaneDistance(const float _value) { m_nearPlaneDist = _value; m_orthographicProjMatDirty = true; m_perspectiveProjMatDirty = true; }
		inline void SetFarPlaneDistance(const float _value)	{ m_farPlaneDist = _value; m_orthographicProjMatDirty = true; m_perspectiveProjMatDirty = true; }
		inline void SetFOV(const float _value) { m_fov = _value; m_orthographicProjMatDirty = true; m_perspectiveProjMatDirty = true; }
		inline void SetAspectRatio(const float _value) { m_aspectRatio = _value; m_orthographicProjMatDirty = true; m_perspectiveProjMatDirty = true; }
		
		SERIALISE_MEMBER_FUNC(m_nearPlaneDist, m_farPlaneDist, m_fov, m_aspectRatio)
		inline void* operator new(const std::size_t size) { return Context::GetAllocator()->Allocate(size, "CerealImplicitCamera", 0, false); }
		inline void operator delete(void* ptr) { Context::GetAllocator()->Free(ptr, false); }
		

	protected:
		//Will update matrices if they're dirty
		[[nodiscard]] glm::mat4 GetViewMatrix(const glm::mat4& _transformationMatrix) const;
		[[nodiscard]] glm::mat4 GetProjectionMatrix(const PROJECTION_METHOD _method);
		
		
		float m_nearPlaneDist;
		float m_farPlaneDist;
		float m_fov;
		float m_aspectRatio;

		glm::mat4 m_orthographicProjMat;
		glm::mat4 m_perspectiveProjMat;
		//Need two flags even though they're based on the same state so we don't have to unnecessarily recalculate, for example, orthographic matrix if user is only requesting perspective matrix
		bool m_orthographicProjMatDirty{ true }; //True if m_fov, m_aspectRatio, m_nearPlaneDist, or m_farPlaneDist has been changed
		bool m_perspectiveProjMatDirty{ true }; //True if m_fov, m_aspectRatio, m_nearPlaneDist, or m_farPlaneDist has been changed

		ViewFrustum m_frustum;
	};
	
}