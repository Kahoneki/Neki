#pragma once

#include "CImGuiInspectorRenderable.h"


namespace NK
{
    
    //Empty tag component to mark an object as being selected for the ImGuizmo rendering
    struct CSelected
    {
    public:
        [[nodiscard]] inline static std::string GetStaticName() { return "Selected"; }

        SERIALISE_EMPTY
    };
    
}
