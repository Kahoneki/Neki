#pragma once

#include <Types/NekiTypes.h>


namespace NK
{
	
	
	class ObjectVsBroadPhaseLayerFilterImpl final : public JPH::ObjectVsBroadPhaseLayerFilter
	{
	public:
		virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
		{
			//todo: is it worth actually writing logic for this
			return true;
		}
	};
	
}