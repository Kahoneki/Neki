#include "Camera.h"

#include <stdexcept>
#include <utility>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace NK
{

	Camera::Camera(glm::vec3 _pos, float _yaw, float _pitch, float _nearPlaneDist, float _farPlaneDist, float _fov, float _aspectRatio)
	: m_pos(_pos), m_yaw(_yaw), m_pitch(_pitch), m_nearPlaneDist(_nearPlaneDist), m_farPlaneDist(_farPlaneDist), m_fov(_fov), m_aspectRatio(_aspectRatio)
	{
		m_viewMatDirty = true;
		m_orthographicProjMatDirty = true;
		m_perspectiveProjMatDirty = true;
		UpdateCameraVectors();
	}



	glm::mat4 Camera::GetViewMatrix()
	{
		if (m_viewMatDirty)
		{
			UpdateCameraVectors();
			m_viewMat = glm::lookAtLH(m_pos, m_pos + m_forward, m_up);
			m_viewMatDirty = false;
		}
		
		return m_viewMat;
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
			}
			return m_perspectiveProjMat;
		}
			
		default:
		{
			throw std::runtime_error("Camera::GetProjectionMatrix() default case for switch on _method reached (_method = " + std::to_string(std::to_underlying(_method)) + "\n");
		}
		}
	}



	CameraShaderData Camera::GetCameraShaderData(const PROJECTION_METHOD _method)
	{
		CameraShaderData camData{};
		camData.viewMat = GetViewMatrix();
		camData.projMat = GetProjectionMatrix(_method);
		return camData;
	}



	void Camera::UpdateCameraVectors()
	{
		//Calculate new forward vector
		glm::vec3 front;
		front.x = static_cast<float>(cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch)));
		front.y = static_cast<float>(sin(glm::radians(m_pitch)));
		front.z = static_cast<float>(sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch)));
		m_forward = glm::normalize(front);

		//Re-calculate right and up vectors
		constexpr glm::vec3 worldUp{ glm::vec3(0, 1, 0) };
		m_right = glm::normalize(glm::cross(worldUp, m_forward));
		m_up = glm::normalize(glm::cross(m_forward, m_right)); 
	}
}