#include "PhysicsLayer.h"

#include <Components/CBoxCollider.h>
#include <Components/CPhysicsBody.h>
#include <Components/CTransform.h>

#include <Jolt/RegisterTypes.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/MotionProperties.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>

#include "Core/Utils/EnumUtils.h"


namespace NK
{

	JPH::Vec3 GLMToJPH(const glm::vec3& _v) { return JPH::Vec3(_v.x, _v.y, _v.z); }
	glm::vec3 JPHToGLM(const JPH::Vec3& _v) { return glm::vec3(_v.GetX(), _v.GetY(), _v.GetZ()); }
	JPH::Quat GLMToJPH(const glm::quat& _q) { return JPH::Quat(_q.x, _q.y, _q.z, _q.w).Normalized(); }
	glm::quat JPHToGLM(const JPH::Quat& _q) { return glm::normalize(glm::quat(_q.GetW(), _q.GetX(), _q.GetY(), _q.GetZ())); }



	PhysicsLayer::PhysicsLayer(Registry& _reg, const PhysicsLayerDesc& _desc) : ILayer(_reg), m_broadPhaseInterface(_desc), m_objectFilter(_desc)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::PHYSICS_LAYER, "Initialising Physics Layer\n");

		m_contactListener.registry = &_reg;
		m_physicsSystem.SetContactListener(&m_contactListener);
		
		JPH::RegisterDefaultAllocator();
		JPH::Factory::sInstance = new JPH::Factory();
		JPH::RegisterTypes();

		m_tempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024); //10MiB scratch
		m_jobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

		m_physicsSystem.Init(1024, 0, 1024, 1024, m_broadPhaseInterface, m_objectBroadPhaseFilter, m_objectFilter);

		m_logger.Unindent();
	}



	void PhysicsLayer::FixedUpdate()
	{
		JPH::BodyInterface& bodyInterface{ m_physicsSystem.GetBodyInterface() };
		
		//Initialise any new bodies
		for (auto&& [body, box, transform] : m_reg.get().View<CPhysicsBody, CBoxCollider, CTransform>())
		{
			if (body.bodyID == 0xFFFFFFFF)
			{
				JPH::BoxShapeSettings shapeSettings{ GLMToJPH(box.halfExtents) };
				JPH::ShapeSettings::ShapeResult shapeResult{ shapeSettings.Create() };

				JPH::EMotionType motionType = JPH::EMotionType::Dynamic;
				if (body.initialMotionType == MOTION_TYPE::STATIC) motionType = JPH::EMotionType::Static;
				else if (body.initialMotionType == MOTION_TYPE::KINEMATIC) motionType = JPH::EMotionType::Kinematic;

				JPH::BodyCreationSettings creationSettings{ shapeResult.Get(), GLMToJPH(transform.GetPosition()), GLMToJPH(transform.GetRotationQuat()), motionType, body.initialObjectLayer.GetValue() };
				creationSettings.mFriction = body.GetFriction();
				creationSettings.mRestitution = body.GetRestitution();
				creationSettings.mLinearDamping = body.GetLinearDamping();
				creationSettings.mAngularDamping = body.GetAngularDamping();
				creationSettings.mGravityFactor = body.GetGravityFactor();
				creationSettings.mIsSensor = body.initialTrigger;
				creationSettings.mLinearVelocity = GLMToJPH(body.initialLinearVelocity);
				creationSettings.mAngularVelocity = GLMToJPH(body.initialAngularVelocity);

				if (body.initialMotionQuality == MOTION_QUALITY::LINEAR_CAST)
				{
					creationSettings.mMotionQuality = JPH::EMotionQuality::LinearCast;
				}

				if (body.GetMass() > 0.0f)
				{
					creationSettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
					creationSettings.mMassPropertiesOverride.mMass = body.GetMass();
				}
				else
				{
					creationSettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateMassAndInertia;
				}

				creationSettings.mUserData = static_cast<std::uint64_t>(m_reg.get().GetEntity(transform));
				
				const JPH::Body* jphBody{ bodyInterface.CreateBody(creationSettings) };
				body.bodyID = jphBody->GetID().GetIndexAndSequenceNumber();
				bodyInterface.AddBody(jphBody->GetID(), JPH::EActivation::Activate);
			}
			
			if (transform.physicsSyncDirty)
			{
				//The transform was updated by something other than the jolt->ctransform sync (e.g.: imgui), perform a ctransform->jolt sync
				bodyInterface.SetPositionAndRotation(JPH::BodyID(body.bodyID), GLMToJPH(transform.GetPosition()), GLMToJPH(transform.GetRotationQuat()), JPH::EActivation::Activate);
				
				//Zero out the velocity to avoid a large jump
				bodyInterface.SetLinearAndAngularVelocity(JPH::BodyID(body.bodyID), JPH::Vec3::sZero(), JPH::Vec3::sZero());
				
				transform.physicsSyncDirty = false;
			}

			if (body.dirtyFlags != PHYSICS_DIRTY_FLAGS::CLEAN)
			{
				JPH::BodyID id(body.bodyID);
				JPH::Body* jphBody{ m_physicsSystem.GetBodyLockInterfaceNoLock().TryGetBody(id) };
				if (EnumUtils::Contains(body.dirtyFlags, PHYSICS_DIRTY_FLAGS::FRICTION)) bodyInterface.SetFriction(id, body.GetFriction());
				if (EnumUtils::Contains(body.dirtyFlags, PHYSICS_DIRTY_FLAGS::RESTITUTION)) bodyInterface.SetRestitution(id, body.GetRestitution());
				if (EnumUtils::Contains(body.dirtyFlags, PHYSICS_DIRTY_FLAGS::GRAVITY)) bodyInterface.SetGravityFactor(id, body.GetGravityFactor());
				if (EnumUtils::Contains(body.dirtyFlags, PHYSICS_DIRTY_FLAGS::DAMPING))
				{
					if (JPH::MotionProperties* mp{ jphBody->GetMotionProperties() })
					{
						mp->SetLinearDamping(body.linearDamping);
						mp->SetAngularDamping(body.angularDamping);
					}
				}
				if (EnumUtils::Contains(body.dirtyFlags, PHYSICS_DIRTY_FLAGS::MASS))
				{
					if (JPH::MotionProperties* mp{ jphBody->GetMotionProperties() })
					{
						JPH::MassProperties mpScaled{ jphBody->GetShape()->GetMassProperties() };
						mpScaled.ScaleToMass(body.GetMass());
						mp->SetMassProperties(JPH::EAllowedDOFs::All, mpScaled);
					}
				}
				body.dirtyFlags = PHYSICS_DIRTY_FLAGS::CLEAN;
			}
		}
		
		//Step the world
		m_physicsSystem.Update(Context::GetFixedUpdateTimestep(), 4, m_tempAllocator, m_jobSystem);
		
		//Sync jolt -> ctransform
		for (auto&& [body, transform] : m_reg.get().View<CPhysicsBody, CTransform>())
		{
			if (body.bodyID == 0xFFFFFFFF || body.initialMotionType != MOTION_TYPE::DYNAMIC)
			{
				continue;
			}

			transform.SyncPosition(JPHToGLM(bodyInterface.GetPosition(JPH::BodyID(body.bodyID))));
			transform.SyncRotation(JPHToGLM(bodyInterface.GetRotation(JPH::BodyID(body.bodyID))));
		}
	}

}
