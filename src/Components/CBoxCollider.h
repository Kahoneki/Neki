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
		
		[[nodiscard]] inline static std::string GetStaticName() { return "Box Collider"; }
		
		SERIALISE_MEMBER_FUNC(halfExtents)
		
		
	private:
		virtual inline std::string GetComponentName() const override { return GetStaticName(); }
		virtual inline ImGuiTreeNodeFlags GetTreeNodeFlags() const override { return ImGuiTreeNodeFlags_DefaultOpen; }
		virtual void RenderImGuiInspectorContents(Registry& _reg) override;
		
		
		bool halfExtentsEditedInInspector{ false }; //setting halfExtentsDirty is expensive, so we want to avoid it for continuous updates (i.e.: ImGui::DragFloat3) - use this if the half extents have been edited but the apply button hasn't been pressed yet
		bool halfExtentsDirty{ true };
		glm::vec3 halfExtents{ 0.5f, 0.5f, 0.5f };
	};
	
}
