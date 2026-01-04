#pragma once

#include "ILayer.h"

#include <Physics/BroadPhaseLayerInterfaceImpl.h>
#include <Physics/ContactListenerImpl.h>
#include <Physics/ObjectLayerPairFilterImpl.h>
#include <Physics/ObjectVsBroadPhaseLayerFilterImpl.h>

#ifdef AddJob
	#undef AddJob
#endif
#include <Jolt/Core/JobSystemThreadPool.h>



namespace NK
{
	
	class PhysicsLayer final : public ILayer
	{
	public:
		explicit PhysicsLayer(Registry& _reg, const PhysicsLayerDesc& _desc);
		virtual ~PhysicsLayer() override;

		virtual void FixedUpdate() override;
		virtual void SetRegistry(Registry& _reg) override;
		
		
	private:
		void OnEntityDestroy(const EntityDestroyEvent& _event);
		void OnComponentRemove(const ComponentRemoveEvent& _event);
		void OnComponentAdd(const ComponentAddEvent& _event);
		
		static JPH::EMotionType GetJPHMotionType(const MOTION_TYPE _type);
		static JPH::EMotionQuality GetJPHMotionQuality(const MOTION_QUALITY _quality);
		
		
		JPH::PhysicsSystem m_physicsSystem;
		JPH::TempAllocatorImpl* m_tempAllocator;
		JPH::JobSystemThreadPool* m_jobSystem;
		BroadPhaseLayerInterfaceImpl m_broadPhaseInterface;
		ContactListenerImpl m_contactListener;
		ObjectLayerPairFilterImpl m_objectFilter;
		ObjectVsBroadPhaseLayerFilterImpl m_objectBroadPhaseFilter;
		
		EventSubscriptionID m_entityDestroyEventSubscriptionID;
		EventSubscriptionID m_componentRemoveEventSubscriptionID;
		EventSubscriptionID m_componentAddEventSubscriptionID;
	};

}