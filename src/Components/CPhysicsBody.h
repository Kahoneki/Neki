#pragma once

#include <Physics/PhysicsObjectLayer.h>


namespace NK
{

	struct CPhysicsBody final
	{
		friend class PhysicsLayer;
		
		
	public:
		PhysicsObjectLayer objectLayer{ UINT16_MAX, PhysicsBroadPhaseLayer(UINT8_MAX) };
		bool dynamic{ false };
		
		
	private:
		std::uint32_t bodyID{ UINT32_MAX };
	};
	
}