#pragma once

#include "CImGuiInspectorRenderable.h"

#include <Core/Memory/Allocation.h>
#include <Graphics/Lights/Light.h>
#include <Graphics/Lights/DirectionalLight.h>
#include <Graphics/Lights/PointLight.h>
#include <Graphics/Lights/SpotLight.h>


namespace NK
{
	
	struct CLight final : public CImGuiInspectorRenderable
	{
	public:
		CLight() : lightType(LIGHT_TYPE::UNDEFINED), light(nullptr) {}
		//Deep copy constructor
		CLight(const CLight& _other) : lightType(_other.lightType)
		{
			if (_other.light)
			{
				switch (lightType)
				{
				case LIGHT_TYPE::DIRECTIONAL:
					light = UniquePtr<Light>(NK_NEW(DirectionalLight, *static_cast<DirectionalLight*>(_other.light.get())));
					break;
				case LIGHT_TYPE::POINT:
					light = UniquePtr<Light>(NK_NEW(PointLight, *static_cast<PointLight*>(_other.light.get())));
					break;
				case LIGHT_TYPE::SPOT:
					light = UniquePtr<Light>(NK_NEW(SpotLight, *static_cast<SpotLight*>(_other.light.get())));
					break;
				default:
					light = nullptr;
					break;
				}
			}
			else
			{
				light = nullptr;
			}
		}
		//Deep copy assignment
		CLight& operator=(const CLight& _other)
		{
			if (this == &_other) { return *this; }

			lightType = _other.lightType;
			light.reset();

			if (_other.light)
			{
				switch (lightType)
				{
				case LIGHT_TYPE::DIRECTIONAL:
					light = UniquePtr<Light>(NK_NEW(DirectionalLight, *static_cast<DirectionalLight*>(_other.light.get())));
					break;
				case LIGHT_TYPE::POINT:
					light = UniquePtr<Light>(NK_NEW(PointLight, *static_cast<PointLight*>(_other.light.get())));
					break;
				case LIGHT_TYPE::SPOT:
					light = UniquePtr<Light>(NK_NEW(SpotLight, *static_cast<SpotLight*>(_other.light.get())));
					break;
				default:
					break;
				}
			}
			
			return *this;
		}
		CLight(CLight&& _other) noexcept : lightType(_other.lightType), light(std::move(_other.light))
		{
			_other.lightType = LIGHT_TYPE::UNDEFINED;
		}
		CLight& operator=(CLight&& _other) noexcept
		{
			if (this == &_other) { return *this; }
			lightType = _other.lightType;
			light = std::move(_other.light);
			_other.lightType = LIGHT_TYPE::UNDEFINED;
			return *this;
		}
		~CLight() override = default;

		
		[[nodiscard]] inline static std::string GetStaticName() { return "Light"; }
		
		[[nodiscard]] inline LIGHT_TYPE GetLightType() const { return lightType; }
		
		
		inline void SetLightType(const LIGHT_TYPE _type)
		{
			//Need to recreate light
					
			//Preserve existing properties if existing
			const glm::vec3 oldColour{ light ? light->GetColour() : glm::vec3(1.0f) };
			const float oldIntensity{ light ? light->GetIntensity() : 1.0f };
			const glm::vec3 oldAttenuation{ (lightType == LIGHT_TYPE::POINT || lightType == LIGHT_TYPE::SPOT) ? glm::vec3(dynamic_cast<PointLight*>(light.get())->GetConstantAttenuation(), dynamic_cast<PointLight*>(light.get())->GetLinearAttenuation(), dynamic_cast<PointLight*>(light.get())->GetQuadraticAttenuation()) : glm::vec3(0.0f) };

			lightType = _type;

			switch (lightType)
			{
			case LIGHT_TYPE::DIRECTIONAL:
				light = UniquePtr<Light>(NK_NEW(DirectionalLight));
				break;
			case LIGHT_TYPE::POINT:
				light = UniquePtr<Light>(NK_NEW(PointLight));
				break;
			case LIGHT_TYPE::SPOT:
				light = UniquePtr<Light>(NK_NEW(SpotLight));
				break;
			default:
				light = nullptr;
				break;
			}

			if (light)
			{
				light->SetColour(oldColour);
				light->SetIntensity(oldIntensity);
				if (lightType == LIGHT_TYPE::POINT || lightType == LIGHT_TYPE::SPOT)
				{
					dynamic_cast<PointLight*>(light.get())->SetConstantAttenuation(oldAttenuation.x);
					dynamic_cast<PointLight*>(light.get())->SetConstantAttenuation(oldAttenuation.y);
					dynamic_cast<PointLight*>(light.get())->SetConstantAttenuation(oldAttenuation.z);
				}
			}
		}
		
		
		SERIALISE_MEMBER_FUNC(lightType, light);
		
		
		UniquePtr<Light> light;
		
		
	private:
		virtual inline std::string GetComponentName() const override { return GetStaticName(); }
		virtual inline ImGuiTreeNodeFlags GetTreeNodeFlags() const override { return ImGuiTreeNodeFlags_DefaultOpen; }
		virtual inline void RenderImGuiInspectorContents(Registry& _reg) override
		{
			const char* lightTypeNames[]{ "Undefined", "Directional", "Point", "Spot" };
			int currentTypeIndex{ static_cast<int>(lightType) };
			if (ImGui::Combo("Type", &currentTypeIndex, lightTypeNames, IM_ARRAYSIZE(lightTypeNames)))
			{
				const LIGHT_TYPE newType{ static_cast<LIGHT_TYPE>(currentTypeIndex) };
				if (newType != lightType)
				{
					SetLightType(newType);
				}
			}

			if (lightType == LIGHT_TYPE::UNDEFINED || !light)
			{
				return;
			}
			
			//Colour
			glm::vec3 color = light->GetColour();
			if (ImGui::ColorEdit3("Colour", &color.x))
			{
				light->SetColour(color);
			}

			//Intensity
			float intensity = light->GetIntensity();
			if (ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.0f, 100.0f))
			{
				light->SetIntensity(intensity);
			}

			if (PointLight* pointLight{ dynamic_cast<PointLight*>(light.get()) })
			{
				//Attenuation
				float constant{ pointLight->GetConstantAttenuation() };
				if (ImGui::DragFloat("Constant Attenuation", &constant, 0.01f, 0.0f, 100.0f)) { pointLight->SetConstantAttenuation(constant); }
				float linear{ pointLight->GetLinearAttenuation() };
				if (ImGui::DragFloat("Linear Attenuation", &linear, 0.01f, 0.0f, 100.0f)) { pointLight->SetLinearAttenuation(linear); }
				float quadratic{ pointLight->GetQuadraticAttenuation() };
				if (ImGui::DragFloat("Quadratic Attenuation", &quadratic, 0.01f, 0.0f, 100.0f)) { pointLight->SetQuadraticAttenuation(quadratic); }

				if (SpotLight* spotLight{ dynamic_cast<SpotLight*>(pointLight) })
				{
					//Angles
					float innerAngle{ glm::degrees(spotLight->GetInnerAngle()) };
					if (ImGui::DragFloat("Inner Angle (degrees)", &innerAngle, 0.01f, 0.0f, 360.0f)) { spotLight->SetInnerAngle(glm::radians(innerAngle)); }
					float outerAngle{ glm::degrees(spotLight->GetOuterAngle()) };
					if (ImGui::DragFloat("Outer Angle (degrees)", &outerAngle, 0.01f, 0.0f, 360.0f)) { spotLight->SetOuterAngle(glm::radians(outerAngle)); }
				}
			}
		}
		
		
		LIGHT_TYPE lightType{ LIGHT_TYPE::UNDEFINED };
	};
	
}
