#pragma once

#include "CImGuiInspectorRenderable.h"
#include "CModelRenderer.h"

#include <glm/glm.hpp>



namespace NK
{
	
	struct CBoxCollider final : public CImGuiInspectorRenderable
	{
		friend struct CTransform;
		friend class PhysicsLayer;
		friend class RenderLayer;
		
		
	public:
		[[nodiscard]] inline glm::vec3 GetHalfExtents() const { return halfExtents; }
		
		//Note: quite expensive, use sparingly (e.g.: avoid calling every frame for continuous updates, just set once at end of updates instead)
		inline void SetHalfExtents(const glm::vec3 _val) { halfExtents = _val; halfExtentsEditedInInspector = false; halfExtentsDirty = true; }
		
		
	private:
		virtual inline std::string GetComponentName() const override { return "Box Collider"; }
		virtual inline ImGuiTreeNodeFlags GetTreeNodeFlags() const override { return ImGuiTreeNodeFlags_DefaultOpen; }
		virtual inline void RenderImGuiInspectorContents(Registry& _reg, CImGuiInspectorRenderable* _component) override
		{
			CBoxCollider* box{ dynamic_cast<CBoxCollider*>(_component) };
			if (ImGui::DragFloat3("Half Extents", &box->halfExtents.x, 0.05f)) { box->halfExtentsEditedInInspector = true; }
			if (box->halfExtentsEditedInInspector)
			{
				if (ImGui::Button("Apply"))
				{
					box->halfExtentsEditedInInspector = false;
					box->halfExtentsDirty = true;
				}
			}
			const Entity entity{ _reg.GetEntity(*box) };
			if (_reg.HasComponent<CModelRenderer>(entity))
			{
				if (ImGui::Button("Match Model Bounds"))
				{
					SetHalfExtents(_reg.GetComponent<CModelRenderer>(entity).localSpaceHalfExtents);
				}
			}
		}
		
		
		bool halfExtentsEditedInInspector{ false }; //setting halfExtentsDirty is expensive, so we want to avoid it for continuous updates (i.e.: ImGui::DragFloat3) - use this if the half extents have been edited but the apply button hasn't been pressed yet
		bool halfExtentsDirty{ false };
		glm::vec3 halfExtents{ 0.5f, 0.5f, 0.5f };
	};
	
}
