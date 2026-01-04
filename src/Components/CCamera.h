#pragma once

#include "CImGuiInspectorRenderable.h"

#include <Graphics/Camera/Camera.h>
#include <Graphics/Camera/PlayerCamera.h>


namespace NK
{

	struct CCamera final : public CImGuiInspectorRenderable
	{
	public:
		CCamera()
		{
			SetCameraType(CAMERA_TYPE::CAMERA);
		}

		CCamera(const CCamera& _other) : enableDOF(_other.enableDOF), focalDistance(_other.focalDistance), focalDepth(_other.focalDepth), maxBlurRadius(_other.maxBlurRadius), dofDebugMode(_other.dofDebugMode), acesExposure(_other.acesExposure), maxIrradiance(_other.maxIrradiance)
		{
			if (_other.camera)
			{
				if (PlayerCamera* pc{ dynamic_cast<PlayerCamera*>(_other.camera.get()) })
				{
					camera = UniquePtr<Camera>(NK_NEW(PlayerCamera, *pc));
				}
				else
				{
					camera = UniquePtr<Camera>(NK_NEW(Camera, *_other.camera));
				}
			}
		}

		CCamera& operator=(const CCamera& _other)
		{
			if (this == &_other) { return *this; }

			enableDOF = _other.enableDOF;
			focalDistance = _other.focalDistance;
			focalDepth = _other.focalDepth;
			maxBlurRadius = _other.maxBlurRadius;
			dofDebugMode = _other.dofDebugMode;
			acesExposure = _other.acesExposure;
			maxIrradiance = _other.maxIrradiance;

			camera.reset();
			if (_other.camera)
			{
				if (PlayerCamera* pc{ dynamic_cast<PlayerCamera*>(_other.camera.get()) })
				{
					camera = UniquePtr<Camera>(NK_NEW(PlayerCamera, *pc));
				}
				else
				{
					camera = UniquePtr<Camera>(NK_NEW(Camera, *_other.camera));
				}
			}

			return *this;
		}

		CCamera(CCamera&&) = default;
		CCamera& operator=(CCamera&&) = default;
		
		
		[[nodiscard]] inline bool GetEnableDepthOfField() const { return enableDOF; }
		[[nodiscard]] inline float GetFocalDistance() const { return focalDistance; }
		[[nodiscard]] inline float GetFocalDepth() const { return focalDepth; }
		[[nodiscard]] inline float GetMaxBlurRadius() const { return maxBlurRadius; }
		[[nodiscard]] inline bool GetDOFDebugMode() const { return dofDebugMode; }
		[[nodiscard]] inline float GetACESExposure() const { return acesExposure; }
		[[nodiscard]] inline float GetMaxIrradiance() const { return maxIrradiance; }
		
		inline void SetEnableDepthOfField(const bool _val) { enableDOF = _val; }
		inline void SetFocalDistance(const float _val) { focalDistance = _val; }
		inline void SetFocalDepth(const float _val) { focalDepth = _val; }
		inline void SetMaxBlurRadius(const float _val) { maxBlurRadius = _val; }
		inline void SetDOFDebugMode(const bool _val) { dofDebugMode = _val; }
		inline void SetACESExposure(const float _val) { acesExposure = _val; }
		inline void SetMaxIrradiance(const float _val) { maxIrradiance = _val; }
		
		[[nodiscard]] inline static std::string GetStaticName() { return "Camera"; }
		
		SERIALISE_MEMBER_FUNC(camera, enableDOF, focalDistance, focalDepth, maxBlurRadius, acesExposure, maxIrradiance)
		
		
		inline void SetCameraType(const CAMERA_TYPE _type)
		{
			const glm::vec3 oldPos = camera ? camera->GetPosition() : glm::vec3(0.0f);
			const float oldYaw = camera ? camera->GetYaw() : 0.0f;
			const float oldPitch = camera ? camera->GetPitch() : 0.0f;
			const float oldNear = camera ? camera->GetNearPlaneDistance() : 0.1f;
			const float oldFar = camera ? camera->GetFarPlaneDistance() : 1000.0f;
			const float oldFov = camera ? camera->GetFOV() : 90.0f;
			const float oldAspect = camera ? camera->GetAspectRatio() : WIN_ASPECT_RATIO;

			camera.reset();

			cameraType = _type;
			
			switch (_type)
			{
			case CAMERA_TYPE::CAMERA:
				camera = UniquePtr<Camera>(NK_NEW(Camera, oldPos, oldYaw, oldPitch, oldNear, oldFar, oldFov, oldAspect));
				break;
			case CAMERA_TYPE::PLAYER_CAMERA:
				//Default values: speed = 30.0f, sensitivity = 0.05f
				camera = UniquePtr<Camera>(NK_NEW(PlayerCamera, oldPos, oldYaw, oldPitch, oldNear, oldFar, oldFov, oldAspect, 30.0f, 0.05f));
				break;
			default:
				break;
			}
		}
		
		
		UniquePtr<Camera> camera;
		
		
	private:
		virtual inline std::string GetComponentName() const override { return GetStaticName(); }
		virtual inline ImGuiTreeNodeFlags GetTreeNodeFlags() const override { return ImGuiTreeNodeFlags_DefaultOpen; }
		virtual inline void RenderImGuiInspectorContents(Registry& _reg) override
		{
			if (camera == nullptr)
			{
				return;
			}
			
			const char* cameraTypeNames[]{ "Camera", "Player Camera" };
			int currentTypeIndex{ static_cast<int>(cameraType) };
			if (ImGui::Combo("Type", &currentTypeIndex, cameraTypeNames, IM_ARRAYSIZE(cameraTypeNames)))
			{
				const CAMERA_TYPE newType{ static_cast<CAMERA_TYPE>(currentTypeIndex) };
				if (newType != cameraType)
				{
					SetCameraType(newType);
				}
			}
			
			//FOV
			float fov = camera->GetFOV();
			if (ImGui::DragFloat("FOV", &fov, 0.1f, 1.0f, 179.0f))
			{
				camera->SetFOV(fov);
			}

			//Near Plane
			float nearPlane = camera->GetNearPlaneDistance();
			if (ImGui::DragFloat("Near Plane", &nearPlane, 0.01f, 0.001f, 10.0f))
			{
				camera->SetNearPlaneDistance(nearPlane);
			}

			//Far Plane
			float farPlane = camera->GetFarPlaneDistance();
			if (ImGui::DragFloat("Far Plane", &farPlane, 1.0f, 1.0f, 10000.0f))
			{
				camera->SetFarPlaneDistance(farPlane);
			}

			if (PlayerCamera* playerCamera{ dynamic_cast<PlayerCamera*>(camera.get()) })
			{
				//Movement Speed
				float speed = playerCamera->GetMovementSpeed();
				if (ImGui::DragFloat("Movement Speed", &speed, 0.1f, 0.0f, 1000.0f))
				{
					playerCamera->SetMovementSpeed(speed);
				}

				//Mouse Sensitivity
				float sensitivity = playerCamera->GetMouseSensitivity();
				if (ImGui::DragFloat("Mouse Sensitivity", &sensitivity, 0.01f, 0.0f, 10.0f))
				{
					playerCamera->SetMouseSensitivity(sensitivity);
				}
			}
			
			if (ImGui::CollapsingHeader("Post-Processing"))
			{
				ImGui::Indent();
				
				ImGui::Checkbox("Depth of Field", &enableDOF);
				if (enableDOF)
				{
					if (ImGui::DragFloat("Focal Distance", &focalDistance, 0.1f, 0.0f, 1000.0f)) {}
					if (ImGui::DragFloat("Focal Depth", &focalDepth, 0.1f, 0.0f, 1000.0f)) {}
					if (ImGui::DragFloat("Max Blur Radius", &maxBlurRadius, 0.1f, 0.0f, 100.0f)) {}
					ImGui::Checkbox("Debug", &dofDebugMode);
				}
				
				if (ImGui::DragFloat("Exposure", &acesExposure, 0.01f, 0.0f, 10.0f)) {}
				if (ImGui::DragFloat("Maximum Irradiance", &maxIrradiance, 0.1f, 0.0f)) {}
				
				ImGui::Unindent();
			}
		}
		
		
		CAMERA_TYPE cameraType;
		bool enableDOF{ false };
		float focalDistance{ 0.0f };
		float focalDepth{ 0.0f };
		float maxBlurRadius{ 0.0f };
		bool dofDebugMode{ false };
		float acesExposure{ 1.0f };
		float maxIrradiance{ 10.0f };
	};
	
}
