#include "Camera.h"

#include <stdexcept>
#include <utility>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace NK
{

	Camera::Camera(glm::vec3 _pos, glm::vec3 _up, float _yaw, float _pitch, float _nearPlaneDist, float _farPlaneDist, float _fov, float _aspectRatio)
	: m_pos(_pos), m_up(_up), m_yaw(_yaw), m_pitch(_pitch), m_nearPlaneDist(_nearPlaneDist), m_farPlaneDist(_farPlaneDist), m_fov(_fov), m_aspectRatio(_aspectRatio)
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
			m_viewMat = glm::lookAt(m_pos, m_pos + m_forward, m_up);
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
				m_perspectiveProjMat = glm::perspective(glm::radians(m_fov), m_aspectRatio, m_nearPlaneDist, m_farPlaneDist);
			}
			return m_perspectiveProjMat;
		}
			
		default:
		{
			throw std::runtime_error("Camera::GetProjectionMatrix() default case for switch on _method reached (_method = " + std::to_string(std::to_underlying(_method)) + "\n");
		}
		}
	}



	void Camera::UpdateCameraVectors()
	{
		//Calculate new forward and up vectors
		m_forward.x = static_cast<float>(sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch)));
		m_forward.y = static_cast<float>(sin(glm::radians(m_pitch)));
		m_forward.z = static_cast<float>(-cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch)));
		m_forward = glm::normalize(m_forward);

		constexpr glm::vec3 worldUp{ glm::vec3(0, 1, 0) };
		m_right = glm::normalize(glm::cross(m_forward, worldUp));
		m_up = glm::normalize(glm::cross(m_right, m_forward));
	}
}