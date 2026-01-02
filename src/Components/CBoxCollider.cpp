#include "CBoxCollider.h"

#include <Core-ECS/Registry.h>

namespace NK
{
    
    //.cpp because full registry type is required and including Registry.h in the header creates a circular reference
    
    void CBoxCollider::RenderImGuiInspectorContents(Registry& _reg)
    {
        if (ImGui::DragFloat3("Half Extents", &halfExtents.x, 0.05f)) { halfExtentsEditedInInspector = true; }
        if (halfExtentsEditedInInspector)
        {
            if (ImGui::Button("Apply"))
            {
                halfExtentsEditedInInspector = false;
                halfExtentsDirty = true;
            }
        }
        const Entity entity{ _reg.GetEntity(*this) };
        if (_reg.HasComponent<CModelRenderer>(entity))
        {
            if (ImGui::Button("Match Model Bounds"))
            {
                SetHalfExtents(_reg.GetComponent<CModelRenderer>(entity).localSpaceHalfExtents);
            }
        }
    }
    
}