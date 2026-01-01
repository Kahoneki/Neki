#pragma once

#include "CImGuiInspectorRenderable.h"

#include <Core/Memory/Allocation.h>
#include <Graphics/Lights/Light.h>
#include <Graphics/Lights/PointLight.h>
#include <Graphics/Lights/SpotLight.h>


namespace NK
{
	
	struct CLight final : public CImGuiInspectorRenderable
	{
	public:
		LIGHT_TYPE lightType{ LIGHT_TYPE::UNDEFINED };
		UniquePtr<Light> light;
		
		
	private:
		virtual inline std::string GetComponentName() const override { return (lightType == LIGHT_TYPE::UNDEFINED ? "Undefined Light" : (lightType == LIGHT_TYPE::DIRECTIONAL ? "Directional Light" : (lightType == LIGHT_TYPE::POINT ? "Point Light" : "Spot Light"))); }
		virtual inline ImGuiTreeNodeFlags GetTreeNodeFlags() const override { return ImGuiTreeNodeFlags_DefaultOpen; }
		virtual inline void RenderImGuiInspectorContents(Registry& _reg) override
		{
			if (!light)
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
	};
	
}