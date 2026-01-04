#pragma once

#include <Physics/PhysicsObjectLayer.h>
#include <Types/NekiTypes.h>
#ifdef max
	#undef max
#endif


namespace NK
{
	struct CPhysicsBody final : public CImGuiInspectorRenderable
	{
		friend class PhysicsLayer;
		friend class RenderLayer;

		
	public:
		CPhysicsBody() = default;

		CPhysicsBody(const CPhysicsBody& _other)
			: objectLayer(_other.objectLayer),
			  motionType(_other.motionType),
			  motionQuality(_other.motionQuality),
			  initialTrigger(_other.initialTrigger),
			  initialLinearVelocity(_other.initialLinearVelocity),
			  initialAngularVelocity(_other.initialAngularVelocity),
			  bodyID(UINT32_MAX),
			  dirtyFlags(PHYSICS_DIRTY_FLAGS::CLEAN),
			  mass(_other.mass),
			  friction(_other.friction),
			  restitution(_other.restitution),
			  linearDamping(_other.linearDamping),
			  angularDamping(_other.angularDamping),
			  gravityFactor(_other.gravityFactor),
			  scale(_other.scale)
		{
		}

		CPhysicsBody& operator=(const CPhysicsBody& _other)
		{
			if (this == &_other) return *this;

			objectLayer = _other.objectLayer;
			motionType = _other.motionType;
			motionQuality = _other.motionQuality;
			initialTrigger = _other.initialTrigger;
			initialLinearVelocity = _other.initialLinearVelocity;
			initialAngularVelocity = _other.initialAngularVelocity;

			mass = _other.mass;
			friction = _other.friction;
			restitution = _other.restitution;
			linearDamping = _other.linearDamping;
			angularDamping = _other.angularDamping;
			gravityFactor = _other.gravityFactor;
			scale = _other.scale;

			bodyID = UINT32_MAX;
			dirtyFlags = PHYSICS_DIRTY_FLAGS::CLEAN;
			forceQueue = std::queue<ForceDesc>();

			return *this;
		}
		
		CPhysicsBody(CPhysicsBody&& _other) noexcept
			: initialTrigger(_other.initialTrigger),
			  initialLinearVelocity(_other.initialLinearVelocity),
			  initialAngularVelocity(_other.initialAngularVelocity),
			  bodyID(_other.bodyID),
			  dirtyFlags(_other.dirtyFlags),
			  objectLayer(_other.objectLayer),
			  motionType(_other.motionType),
			  motionQuality(_other.motionQuality),
			  mass(_other.mass),
			  friction(_other.friction),
			  restitution(_other.restitution),
			  linearDamping(_other.linearDamping),
			  angularDamping(_other.angularDamping),
			  gravityFactor(_other.gravityFactor),
			  scale(_other.scale),
			  forceQueue(std::move(_other.forceQueue))
		{
			_other.bodyID = UINT32_MAX; 
		}

		CPhysicsBody& operator=(CPhysicsBody&& _other) noexcept
		{
			if (this == &_other) return *this;

			objectLayer = _other.objectLayer;
			motionType = _other.motionType;
			motionQuality = _other.motionQuality;
			initialTrigger = _other.initialTrigger;
			initialLinearVelocity = _other.initialLinearVelocity;
			initialAngularVelocity = _other.initialAngularVelocity;

			bodyID = _other.bodyID;
			dirtyFlags = _other.dirtyFlags;
        
			mass = _other.mass;
			friction = _other.friction;
			restitution = _other.restitution;
			linearDamping = _other.linearDamping;
			angularDamping = _other.angularDamping;
			gravityFactor = _other.gravityFactor;
			scale = _other.scale;
        
			forceQueue = std::move(_other.forceQueue);

			_other.bodyID = UINT32_MAX;
        
			return *this;
		}
		
		
		[[nodiscard]] inline PhysicsObjectLayer GetObjectLayer() const { return objectLayer; }
		[[nodiscard]] inline MOTION_TYPE GetMotionType() const { return motionType; }
		[[nodiscard]] inline MOTION_QUALITY GetMotionQuality() const { return motionQuality; }
		[[nodiscard]] inline float GetMass() const { return mass; }
		[[nodiscard]] inline float GetFriction() const { return friction; }
		[[nodiscard]] inline float GetRestitution() const { return restitution; }
		[[nodiscard]] inline float GetLinearDamping() const { return linearDamping; }
		[[nodiscard]] inline float GetAngularDamping() const { return angularDamping; }
		[[nodiscard]] inline float GetGravityFactor() const { return gravityFactor; }
		
		inline void SetObjectLayer(const PhysicsObjectLayer& _val) { objectLayer = _val; dirtyFlags |= PHYSICS_DIRTY_FLAGS::OBJECT_LAYER; }
		inline void SetMotionType(const MOTION_TYPE _val) { motionType = _val; dirtyFlags |= PHYSICS_DIRTY_FLAGS::MOTION_TYPE; }
		inline void SetMotionQuality(const MOTION_QUALITY _val) { motionQuality = _val; dirtyFlags |= PHYSICS_DIRTY_FLAGS::MOTION_QUALITY; }
		inline void SetMass(const float _val) { mass = _val; dirtyFlags |= PHYSICS_DIRTY_FLAGS::MASS; }
		inline void SetFriction(const float _val) { friction = _val; dirtyFlags |= PHYSICS_DIRTY_FLAGS::FRICTION; }
		inline void SetRestitution(const float _val) { restitution = _val; dirtyFlags |= PHYSICS_DIRTY_FLAGS::RESTITUTION; }
		inline void SetLinearDamping(const float _val) { linearDamping = _val; dirtyFlags |= PHYSICS_DIRTY_FLAGS::DAMPING; }
		inline void SetAngularDamping(const float _val) { angularDamping = _val; dirtyFlags |= PHYSICS_DIRTY_FLAGS::DAMPING; }
		inline void SetGravityFactor(const float _val) { gravityFactor = _val; dirtyFlags |= PHYSICS_DIRTY_FLAGS::GRAVITY; }
		
		inline void AddForce(const ForceDesc& _desc) { forceQueue.push(_desc); }
		inline void AddForce(const glm::vec3& _forceVector, const FORCE_MODE _mode = FORCE_MODE::FORCE) { forceQueue.push({ .forceVector = _forceVector, .mode = _mode }); }
		
		
		[[nodiscard]] inline static std::string GetStaticName() { return "Physics Body"; }
		
		SERIALISE_MEMBER_FUNC(motionQuality, initialTrigger, initialLinearVelocity, initialAngularVelocity, objectLayer, motionType, mass, friction, restitution, linearDamping, angularDamping, gravityFactor, scale);

		bool initialTrigger{ false };

		glm::vec3 initialLinearVelocity{ 0.0f, 0.0f, 0.0f };
		glm::vec3 initialAngularVelocity{ 0.0f, 0.0f, 0.0f };

		
	private:
		virtual inline std::string GetComponentName() const override { return GetStaticName(); }
		virtual inline ImGuiTreeNodeFlags GetTreeNodeFlags() const override { return ImGuiTreeNodeFlags_DefaultOpen; }
		virtual inline void RenderImGuiInspectorContents(Registry& _reg) override
		{
			const std::vector<PhysicsObjectLayer>& objectLayers{ TypeRegistry::GetObjectLayers() };
			const PhysicsObjectLayer currentLayer{ GetObjectLayer() };
			const std::string currentLayerName{ currentLayer.GetName() };
			if (ImGui::BeginCombo("Object Layer", currentLayerName.c_str()))
			{
				for (const PhysicsObjectLayer& layer : objectLayers)
				{
					const bool isSelected{ currentLayer.GetValue() == layer.GetValue() };
					if (ImGui::Selectable(layer.GetName().c_str(), isSelected))
					{
						SetObjectLayer(layer);
					}
					if (isSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			
			const char* motionTypes[]{ "Static", "Kinematic", "Dynamic" };
			int currentMotionType{ static_cast<int>(GetMotionType()) };
			if (ImGui::Combo("Motion Type", &currentMotionType, motionTypes, IM_ARRAYSIZE(motionTypes)))
			{
				SetMotionType(static_cast<MOTION_TYPE>(currentMotionType));
			}
			
			const char* motionQualities[]{ "Discrete", "Linear Cast" };
			int currentMotionQuality{ static_cast<int>(GetMotionQuality()) };
			if (ImGui::Combo("Motion Quality", &currentMotionQuality, motionQualities, IM_ARRAYSIZE(motionQualities)))
			{
				SetMotionQuality(static_cast<MOTION_QUALITY>(currentMotionQuality));
			}
			
			float newMass{ GetMass() };
			float newFriction{ GetFriction() };
			float newRestitution{ GetRestitution() };
			float newLinearDamping{ GetLinearDamping() };
			float newAngularDamping{ GetAngularDamping() };
			float newGravityFactor{ GetGravityFactor() };
			if (ImGui::DragFloat("Mass", &newMass, 0.05f)) { SetMass(std::max(0.0f, newMass)); }
			if (ImGui::DragFloat("Friction", &newFriction, 0.05f)) { SetFriction(std::max(0.0f, newFriction)); }
			if (ImGui::DragFloat("Restitution", &newRestitution, 0.05f)) { SetRestitution(std::max(0.0f, newRestitution)); }
			if (ImGui::DragFloat("Linear Damping", &newLinearDamping, 0.05f)) { SetLinearDamping(std::max(0.0f, newLinearDamping)); }
			if (ImGui::DragFloat("Angular Damping", &newAngularDamping, 0.05f)) { SetAngularDamping(std::max(0.0f, newAngularDamping)); }
			if (ImGui::DragFloat("Gravity Factor", &newGravityFactor, 0.05f)) { SetGravityFactor(std::max(0.0f, newGravityFactor)); }
			
			if (ImGui::CollapsingHeader("Force Test"))
			{
				ImGui::Indent();
				
				static glm::vec3 direction{};
				static float strength{};
				const char* modes[]{ "Force", "Impulse" };
				static int selectedMode{ 0 };
				ImGui::InputFloat3("Direction", &direction.x);
				ImGui::DragFloat("Strength", &strength);
				ImGui::Combo("Mode", &selectedMode, modes, IM_ARRAYSIZE(modes));
					
				if (ImGui::Button(selectedMode == 0 ? "Hold to Apply" : "Click to Apply"))
				{
					AddForce(glm::normalize(direction) * strength, static_cast<NK::FORCE_MODE>(selectedMode));
				}
				if (selectedMode == 0 && ImGui::IsItemActive())
				{
					AddForce(glm::normalize(direction) * strength, static_cast<NK::FORCE_MODE>(selectedMode));
				}
				
				ImGui::Unindent();
			}
		}
		
		
		std::uint32_t bodyID{ UINT32_MAX };
		PHYSICS_DIRTY_FLAGS dirtyFlags{ PHYSICS_DIRTY_FLAGS::CLEAN };
		
		PhysicsObjectLayer objectLayer{ DefaultObjectLayer };

		MOTION_TYPE motionType{ MOTION_TYPE::DYNAMIC };
		MOTION_QUALITY motionQuality{ MOTION_QUALITY::DISCRETE };

		float mass{ 0.0f };
		float friction{ 0.2f };
		float restitution{ 0.0f };
		float linearDamping{ 0.05f };
		float angularDamping{ 0.05f };
		float gravityFactor{ 1.0f };
		glm::vec3 scale{ 1.0f, 1.0f, 1.0f };
		
		std::queue<ForceDesc> forceQueue; //Applied and cleared every FixedUpdate in the PhysicsLayer
	};
}
