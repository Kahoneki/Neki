#pragma once

#include <Core-ECS/Registry.h>

#include <imgui.h>


namespace NK
{
    
    //Inherited by any components that are to be rendered in the inspector menu
    struct CImGuiInspectorRenderable
    {
        friend class RenderLayer;
        
        
    public:
        virtual ~CImGuiInspectorRenderable() = default;
        
        
    protected:
        virtual std::string GetComponentName() const = 0;
        virtual ImGuiTreeNodeFlags GetTreeNodeFlags() const { return 0; }
        virtual void RenderImGuiInspectorContents(Registry& _reg) = 0;
    };
    
}
