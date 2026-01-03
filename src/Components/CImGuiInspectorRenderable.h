#pragma once

#include <imgui.h>
#include <string>


namespace NK
{
    
    class Registry;
    
    //Inherited by any components that are to be rendered in the inspector menu
    struct CImGuiInspectorRenderable
    {
        friend class RenderLayer;
        
        
    public:
        virtual ~CImGuiInspectorRenderable() = default;
        
        SERIALISE_MEMBER_FUNC()
        
        
    protected:
        virtual std::string GetComponentName() const = 0;
        virtual ImGuiTreeNodeFlags GetTreeNodeFlags() const { return 0; }
        virtual void RenderImGuiInspectorContents(Registry& _reg) = 0;
    };
    
}
