#include "PhysicsLayer.h"

#include <Components/CBoxCollider.h>
#include <Components/CPhysicsBody.h>
#include <Components/CTransform.h>

#include <Jolt/RegisterTypes.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>


namespace NK
{

	JPH::Vec3 GLMToJPH(const glm::vec3& _v) { return JPH::Vec3(_v.x, _v.y, _v.z); }
	glm::vec3 JPHToGLM(const JPH::Vec3& _v) { return glm::vec3(_v.GetX(), _v.GetY(), _v.GetZ()); }
	JPH::Quat GLMToJPH(const glm::quat& _q) { return JPH::Quat(_q.x, _q.y, _q.z, _q.w); }
	glm::quat JPHToGLM(const JPH::Quat& _q) { return glm::quat(_q.GetX(), _q.GetY(), _q.GetZ(), _q.GetW()); }



	PhysicsLayer::PhysicsLayer(Registry& _reg, const PhysicsLayerDesc& _desc) : ILayer(_reg), m_broadPhaseInterface(_desc), m_objectFilter(_desc)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::PHYSICS_LAYER, "Initialising Physics Layer\n");

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
				JPH::BodyCreationSettings creationSettings{ shapeResult.Get(), GLMToJPH(transform.GetPosition()), GLMToJPH(glm::quat(transform.GetRotation())), (body.dynamic ? JPH::EMotionType::Dynamic : JPH::EMotionType::Static), body.objectLayer.GetValue() };
			
				const JPH::Body* jphBody{ bodyInterface.CreateBody(creationSettings) };
				body.bodyID = jphBody->GetID().GetIndexAndSequenceNumber();
				bodyInterface.AddBody(jphBody->GetID(), JPH::EActivation::Activate);
			}
		}
		
		//Step the world
		m_physicsSystem.Update(Context::GetFixedUpdateTimestep(), 1, m_tempAllocator, m_jobSystem);
		
		//Sync jolt -> ctransform
		for (auto&& [body, transform] : m_reg.get().View<CPhysicsBody, CTransform>())
		{
			if (body.bodyID == 0xFFFFFFFF || !body.dynamic)
			{
				continue;
			}

			transform.SetPosition(JPHToGLM(bodyInterface.GetPosition(JPH::BodyID(body.bodyID))));
			transform.SetRotation(glm::eulerAngles(JPHToGLM(bodyInterface.GetRotation(JPH::BodyID(body.bodyID)))));
		}
	}

}