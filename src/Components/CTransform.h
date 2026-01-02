#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include "CBoxCollider.h"
#include "CImGuiInspectorRenderable.h"

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>


namespace NK
{

	struct CTransform final : public CImGuiInspectorRenderable
	{
		friend class Registry;
		friend class RenderLayer;
		friend class PhysicsLayer;
		
		friend void RenderImGuiInspectorContents(CTransform& _transform);


	public:
		[[nodiscard]] inline CTransform* GetParent() const { return parent; }
		
		[[nodiscard]] inline glm::vec3 GetLocalPosition() const { return localPos; }
		[[nodiscard]] inline glm::vec3 GetLocalRotation() const { return glm::eulerAngles(localRot); }
		[[nodiscard]] inline glm::quat GetLocalRotationQuat() const { return localRot; }
		[[nodiscard]] inline glm::vec3 GetLocalScale() const { return localScale; }
		
		[[nodiscard]] inline glm::vec3 GetWorldPosition() { return glm::vec3(GetModelMatrix()[3]); }
		[[nodiscard]] inline glm::vec3 GetWorldRotation() const { return glm::eulerAngles(GetWorldRotationQuat()); }
		[[nodiscard]] inline glm::quat GetWorldRotationQuat() const { return glm::normalize(parent ? parent->GetWorldRotationQuat() * localRot : localRot); }
		[[nodiscard]] inline glm::vec3 GetWorldScale()
		{
			glm::mat4 m = GetModelMatrix();
			return glm::vec3(glm::length(glm::vec3(m[0])), glm::length(glm::vec3(m[1])), glm::length(glm::vec3(m[2])));
		}
		
		[[nodiscard]] inline glm::mat4 GetModelMatrix()
		{
			if (localMatrixDirty)
			{
				//Scale -> Rotation -> Translation
				//Because matrix multiplication order is reversed, do trans * rot * scale
				const glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), localScale);
				const glm::mat4 rotMat = glm::mat4_cast(localRot);
				const glm::mat4 transMat = glm::translate(glm::mat4(1.0f), localPos);	
				localMatrix = transMat * rotMat * scaleMat;
				localMatrixDirty = false;
			}
			
			if (parent != nullptr)
			{
				return parent->GetModelMatrix() * localMatrix;
			}
			
			return localMatrix;
		}

		
		//Returns true if successful
		//(will return false in the case of an attempted circular parenting)
		inline bool SetParent(Registry& _reg, CTransform* const _parent)
		{
			//Avoid circular parenting
			const CTransform* currentTransform{ _parent };
			while (currentTransform != nullptr)
			{
				if (currentTransform->GetParent() == this)
				{
					//Circular parenting found, disallow
					return false;
				}
				currentTransform = currentTransform->GetParent();
			}

			//Remove from old parent's children vector
			if (parent != nullptr)
			{
				const std::vector<CTransform*>::iterator it{ std::ranges::find(parent->children, this) };
				if (it != parent->children.end())
				{
					parent->children.erase(it);
				}
			}

			const glm::mat4 oldWorldMatrix{ GetModelMatrix() };
			parent = _parent;

			//Calculate new local properties relative to the new parent
			if (parent != nullptr) 
			{
				parent->children.push_back(this);
				const glm::mat4 newLocalMatrix{ glm::inverse(parent->GetModelMatrix()) * oldWorldMatrix };
				glm::vec3 skew;
				glm::vec4 perspective;
				glm::decompose(newLocalMatrix, localScale, localRot, localPos, skew, perspective);
				localRot = glm::normalize(localRot);
			} 
			else 
			{
				//No parent, world space is just local space
				glm::vec3 skew;
				glm::vec4 perspective;
				glm::decompose(oldWorldMatrix, localScale, localRot, localPos, skew, perspective);
			}
			
			localMatrixDirty = true;
			lightBufferDirty = true;
			physicsSyncDirty = true;
			InvalidateChildTree(true, true, true);
			
			return true;
		}
		//Immediately set the local position - note: this will result in any residual linear and angular velocity being zeroed out
		inline void SetLocalPosition(const glm::vec3 _val)
		{
			localPos = _val;
			localMatrixDirty = true;
			lightBufferDirty = true;
			physicsSyncDirty = true;
			InvalidateChildTree(true, true, true);
		}
		//Immediately set the local euler rotation in radians - note: this will result in any residual linear and angular velocity being zeroed out
		inline void SetLocalRotation(const glm::vec3 _val)
		{
			localRot = glm::normalize(glm::quat(_val));
			localMatrixDirty = true;
			lightBufferDirty = true;
			physicsSyncDirty = true;
			InvalidateChildTree(true, true, true);
		}
		//Immediately set the local quaternion rotation - note: this will result in any residual linear and angular velocity being zeroed out
		inline void SetLocalRotation(const glm::quat _val)
		{
			localRot = _val;
			localMatrixDirty = true;
			lightBufferDirty = true;
			physicsSyncDirty = true;
			InvalidateChildTree(true, true, true);
		}
		inline void SetLocalScale(const glm::vec3 _val)
		{
			localScale = _val;
			localMatrixDirty = true;
			lightBufferDirty = true;
			InvalidateChildTree(true, true, false);
		}
		
		//Immediately set the world position - note: this will result in any residual linear and angular velocity being zeroed out
		inline void SetWorldPosition(const glm::vec3 _val) { SetLocalPosition(parent ? glm::vec3(glm::inverse(parent->GetModelMatrix()) * glm::vec4(_val, 1.0f)) : _val); }
		//Immediately set the world euler rotation in radians - note: this will result in any residual linear and angular velocity being zeroed out
		inline void SetWorldRotation(const glm::vec3 _val) { SetWorldRotation(glm::quat(_val));  }
		//Immediately set the world quaternion rotation - note: this will result in any residual linear and angular velocity being zeroed out
		inline void SetWorldRotation(const glm::quat _val) { SetLocalRotation(parent ? glm::inverse(parent->GetWorldRotationQuat()) * _val : _val); }
		inline void SetWorldScale(const glm::vec3 _val) { SetLocalScale(parent ? _val / parent->GetWorldScale() : _val); }
		
		[[nodiscard]] inline static std::string GetStaticName() { return "Transform"; }
		
		
		std::string name{ "Unnamed" };
		
		
	private:
		//To be called by the PhysicsLayer
		inline void SyncPosition(const glm::vec3 _val) { localPos = (parent ? glm::vec3(glm::inverse(parent->GetModelMatrix()) * glm::vec4(_val, 1.0f)) : _val); localMatrixDirty = true; lightBufferDirty = true; }
		//To be called by the PhysicsLayer
		//Set quaternion rotation
		inline void SyncRotation(const glm::quat _val) { localRot = (parent ? glm::inverse(parent->GetWorldRotationQuat()) * _val : _val); localMatrixDirty = true; lightBufferDirty = true;  }
		
		
		inline void InvalidateChildTree(const bool _localMat, const bool _lightBuf, const bool _physicsSync) const
		{
			for (CTransform* child : children)
			{
				child->localMatrixDirty = _localMat;
				child->lightBufferDirty = _lightBuf;
				child->physicsSyncDirty = _physicsSync;
				child->InvalidateChildTree(_localMat, _lightBuf, _physicsSync);
			}
		}
		
		
		virtual inline std::string GetComponentName() const override { return GetStaticName(); }
		virtual inline ImGuiTreeNodeFlags GetTreeNodeFlags() const override { return ImGuiTreeNodeFlags_DefaultOpen; }
		virtual inline void RenderImGuiInspectorContents(Registry& _reg) override
		{
			if (ImGui::RadioButton("Local", local)) { local = true; }
			ImGui::SameLine();
			if (ImGui::RadioButton("World", !local)) { local = false; }
			glm::vec3 pos{ local ? GetLocalPosition() : GetWorldPosition() };
			glm::vec3 rot{ glm::degrees(local ? GetLocalRotation() : GetWorldRotation()) };
			glm::vec3 scale{ local ? GetLocalScale() : GetWorldScale() };
			if (ImGui::DragFloat3("Position", &pos.x, 0.05f)) { local ? SetLocalPosition(pos) : SetWorldPosition(pos); }
			if (ImGui::DragFloat3("Rotation", &rot.x, 0.05f)) { local ? SetLocalRotation(glm::radians(rot)) : SetWorldRotation(rot); }
			if (ImGui::DragFloat3("Scale", &scale.x, 0.05f)) { local ? SetLocalScale(scale) : SetWorldScale(scale); }
			if (ImGui::CollapsingHeader("Matrices"))
			{
				ImGui::Indent();
				ImGui::PushID("Matrices");
				
				if (ImGui::CollapsingHeader("Local"))
				{
					if (ImGui::BeginTable("Local", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) 
					{
						for (int row = 0; row < 4; row++) 
						{
							ImGui::TableNextRow();
							for (int col = 0; col < 4; col++) 
							{
								ImGui::TableSetColumnIndex(col);
								ImGui::Text("%.2f", localMatrix[col][row]);
							}
						}
						ImGui::EndTable();
					}
				}
			
				if (ImGui::CollapsingHeader("World"))
				{
					if (ImGui::BeginTable("World", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) 
					{
						glm::mat4 worldMatrix{ GetModelMatrix() };
						for (int row = 0; row < 4; row++) 
						{
							ImGui::TableNextRow();
							for (int col = 0; col < 4; col++) 
							{
								ImGui::TableSetColumnIndex(col);
								ImGui::Text("%.2f", worldMatrix[col][row]);
							}
						}
						ImGui::EndTable();
					}
				}
				
				ImGui::PopID();
				ImGui::Unindent();
			}
		}
		
		
		CTransform* parent{ nullptr }; //nullptr = no parent
		std::vector<CTransform*> children;
		
		glm::mat4 localMatrix{ glm::mat4(1.0f) };

		//True if pos, rot, and/or scale have been changed but localMatrix hasn't been updated yet
		bool localMatrixDirty{ true };

		//True if pos and/or rot have been changed by anything other than the physics layer's jolt->ctransform sync but the cjolt sync hasn't happened yet
		//In other words, this flag gets set everytime anything other than the physics layer changes the transform, and it marks to the physics layer that the underlying jolt values have to be synced to match
		bool physicsSyncDirty{ true };
		
		//For lights
		//True if pos, rot, and/or scale have been changed but the light buffer hasn't been updated yet by RenderLayer
		bool lightBufferDirty{ true };
		
		glm::vec3 localPos{ glm::vec3(0.0f) };
		glm::quat localRot{ glm::quat(1.0f, 0.0f, 0.0f, 0.0f) }; //identity quaternion (wxyz: .w=1, .xyz=0)
		glm::vec3 localScale{ glm::vec3(1.0f) };
		
		
		//UI
		bool local{ true }; //Local if true, world if false
	};
	
}