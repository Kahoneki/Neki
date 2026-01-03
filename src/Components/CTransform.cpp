#include "CTransform.h"

#include <Core-ECS/Registry.h>


namespace NK
{
    
    void CTransform::OnBeforeSerialise(Registry& _reg)
    {
        if (parent) { serialisedParentID = _reg.GetEntity(*parent); }
        else { serialisedParentID = UINT32_MAX; }
    }
    
}
