#include "Camera.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>


namespace NK
{

	Camera::Camera(float _nearPlaneDist, float _farPlaneDist, float _fov, float _aspectRatio)
	: m_nearPlaneDist(_nearPlaneDist), m_farPlaneDist(_farPlaneDist), m_fov(_fov), m_aspectRatio(_aspectRatio)
	{
		m_orthographicProjMatDirty = true;
		m_perspectiveProjMatDirty = true;
	}



	glm::mat4 Camera::GetViewMatrix(const glm::mat4& _transformationMatrix) const
	{
		return glm::inverse(_transformationMatrix);
	}



	glm::mat4 Camera::GetProjectionMatrix(const PROJECTION_METHOD _method)
	{
		switch (_method)
		{
		case PROJECTION_METHOD::ORTHOGRAPHIC:
		{
			//Not currently working without hacky magic number workarounds
			throw std::runtime_error("Camera::GetProjectionMatrix() called with _method = PROJECTION_METHOD::ORTHOGRAPHIC - this projection method is not currently supported.\n");
			
			if (m_orthographicProjMatDirty)
			{
				//Half the (vertical) fov and use trig to get the distance from origin to top plane
				const float top{ m_nearPlaneDist * static_cast<float>(tan(glm::radians(m_fov / 2.0f))) };
				const float right{ top * m_aspectRatio }; //By definition
				m_orthographicProjMat = glm::ortho(-right, right, -top, top, m_nearPlaneDist, m_farPlaneDist);
				m_orthographicProjMatDirty = false;
			}
			return m_orthographicProjMat;
		}
			
		case PROJECTION_METHOD::PERSPECTIVE:
		{
			if (m_perspectiveProjMatDirty)
			{
				m_perspectiveProjMat = glm::perspectiveLH(glm::radians(m_fov), m_aspectRatio, m_nearPlaneDist, m_farPlaneDist);
				m_perspectiveProjMatDirty = false;
			}
			return m_perspectiveProjMat;
		}
			
		default:
		{
			throw std::runtime_error("Camera::GetProjectionMatrix() default case for switch on _method reached (_method = " + std::to_string(std::to_underlying(_method)) + "\n");
		}
		}
	}



	CameraShaderData Camera::GetCurrentCameraShaderData(const PROJECTION_METHOD _method, const glm::mat4& _transformationMatrix)
	{
		CameraShaderData camData{};
		camData.viewMat = GetViewMatrix(_transformationMatrix);
		camData.projMat = GetProjectionMatrix(_method);
		camData.pos = _transformationMatrix[3];
		return camData;
	}
	
}