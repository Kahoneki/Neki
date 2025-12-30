#pragma once

#include "ILayer.h"

#include <Physics/BroadPhaseLayerInterfaceImpl.h>
#include <Physics/ContactListenerImpl.h>
#include <Physics/ObjectLayerPairFilterImpl.h>
#include <Physics/ObjectVsBroadPhaseLayerFilterImpl.h>

#include <Jolt/Core/JobSystemThreadPool.h>



namespace NK
{
	
	class PhysicsLayer final : public ILayer
	{
	public:
		explicit PhysicsLayer(Registry& _reg, const PhysicsLayerDesc& _desc);
		virtual ~PhysicsLayer() override = default;

		virtual void FixedUpdate() override;
		
		
	private:
		JPH::PhysicsSystem m_physicsSystem;
		JPH::TempAllocatorImpl* m_tempAllocator;
		JPH::JobSystemThreadPool* m_jobSystem;
		BroadPhaseLayerInterfaceImpl m_broadPhaseInterface;
		ContactListenerImpl m_contactListener;
		ObjectLayerPairFilterImpl m_objectFilter;
		ObjectVsBroadPhaseLayerFilterImpl m_objectBroadPhaseFilter;
	};

}