#pragma once

#include <Physics/PhysicsObjectLayer.h>
#include <Types/NekiTypes.h>


namespace NK
{
	struct CPhysicsBody final
	{
		friend class PhysicsLayer;

		
	public:
		[[nodiscard]] inline float GetMass() const { return mass; }
		[[nodiscard]] inline float GetFriction() const { return friction; }
		[[nodiscard]] inline float GetRestitution() const { return restitution; }
		[[nodiscard]] inline float GetLinearDamping() const { return linearDamping; }
		[[nodiscard]] inline float GetAngularDamping() const { return angularDamping; }
		[[nodiscard]] inline float GetGravityFactor() const { return gravityFactor; }
		
		inline void SetMass(const float _val) { mass = _val; dirtyFlags |= PHYSICS_DIRTY_FLAGS::MASS; }
		inline void SetFriction(const float _val) { friction = _val; dirtyFlags |= PHYSICS_DIRTY_FLAGS::FRICTION; }
		inline void SetRestitution(const float _val) { restitution = _val; dirtyFlags |= PHYSICS_DIRTY_FLAGS::RESTITUTION; }
		inline void SetLinearDamping(const float _val) { linearDamping = _val; dirtyFlags |= PHYSICS_DIRTY_FLAGS::DAMPING; }
		inline void SetAngularDamping(const float _val) { angularDamping = _val; dirtyFlags |= PHYSICS_DIRTY_FLAGS::DAMPING; }
		inline void SetGravityFactor(const float _val) { gravityFactor = _val; dirtyFlags |= PHYSICS_DIRTY_FLAGS::GRAVITY; }

		
		PhysicsObjectLayer initialObjectLayer{ UINT16_MAX, PhysicsBroadPhaseLayer(UINT8_MAX) };

		MOTION_TYPE initialMotionType{ MOTION_TYPE::DYNAMIC };
		MOTION_QUALITY initialMotionQuality{ MOTION_QUALITY::DISCRETE };

		bool initialTrigger{ false };

		glm::vec3 initialLinearVelocity{ 0.0f, 0.0f, 0.0f };
		glm::vec3 initialAngularVelocity{ 0.0f, 0.0f, 0.0f };

		
	private:
		std::uint32_t bodyID{ UINT32_MAX };
		PHYSICS_DIRTY_FLAGS dirtyFlags{ PHYSICS_DIRTY_FLAGS::CLEAN };

		float mass{ 0.0f };
		float friction{ 0.2f };
		float restitution{ 0.0f };
		float linearDamping{ 0.05f };
		float angularDamping{ 0.05f };
		float gravityFactor{ 1.0f };
		glm::vec3 scale{ 1.0f, 1.0f, 1.0f };
	};
}
