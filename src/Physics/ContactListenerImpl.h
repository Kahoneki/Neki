#pragma once

#include <Core-ECS/Registry.h>
#include <Types/NekiTypes.h>

#include "Managers/EventManager.h"


namespace NK
{
	
	class ContactListenerImpl final : public JPH::ContactListener
	{
	public:
		Registry* registry;

		virtual JPH::ValidateResult OnContactValidate(const JPH::Body &_inBody1, const JPH::Body &_inBody2, JPH::RVec3Arg _inBaseOffset, const JPH::CollideShapeResult &_inCollisionResult) override
		{
			return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
		}

		virtual void OnContactAdded(const JPH::Body &_inBody1, const JPH::Body &_inBody2, const JPH::ContactManifold &_inManifold, JPH::ContactSettings &_ioSettings) override
		{
			const Entity e1{ static_cast<Entity>(_inBody1.GetUserData()) };
			const Entity e2{ static_cast<Entity>(_inBody2.GetUserData()) };
			EventManager::Trigger(CollisionEvent{ e1, e2 });
		}
	};
	
}
