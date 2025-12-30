#pragma once

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>


namespace NK
{

	struct CTransform final
	{
		friend class RenderLayer;
		friend class PhysicsLayer;


	public:
		[[nodiscard]] inline glm::vec3 GetPosition() const { return pos; }
		[[nodiscard]] inline glm::vec3 GetRotation() const { return glm::eulerAngles(rot); }
		[[nodiscard]] inline glm::quat GetRotationQuat() const { return rot; }
		[[nodiscard]] inline glm::vec3 GetScale() const { return scale; }
		[[nodiscard]] inline glm::mat4 GetModelMatrix()
		{
			if (!modelMatDirty) { return modelMat; }
			//Scale -> Rotation -> Translation
			//Because matrix multiplication order is reversed, do trans * rot * scale

			const glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scale);
			const glm::mat4 rotMat = glm::mat4_cast(rot);
			const glm::mat4 transMat = glm::translate(glm::mat4(1.0f), pos);

			modelMat = transMat * rotMat * scaleMat;

			modelMatDirty = false;

			return modelMat;
		}

		//Immediately set the position - note: this will result in any residual linear and angular velocity being zeroed out
		inline void SetPosition(const glm::vec3 _val) { pos = _val; modelMatDirty = true; lightBufferDirty = true; physicsSyncDirty = true; }
		//Immediately set the euler rotation in radians - note: this will result in any residual linear and angular velocity being zeroed out
		inline void SetRotation(const glm::vec3 _val) { rot = glm::quat(_val); modelMatDirty = true; lightBufferDirty = true; physicsSyncDirty = true; }
		//Immediately set the quaternion rotation - note: this will result in any residual linear and angular velocity being zeroed out
		inline void SetRotation(const glm::quat _val) { rot = _val; modelMatDirty = true; lightBufferDirty = true; physicsSyncDirty = true; }
		inline void SetScale(const glm::vec3 _val) { scale = _val; modelMatDirty = true; lightBufferDirty = true; }
		
		
		
		
	private:
		//To be called by the PhysicsLayer
		inline void SyncPosition(const glm::vec3 _val) { pos = _val; modelMatDirty = true; lightBufferDirty = true; }
		//To be called by the PhysicsLayer
		//Set quaternion rotation
		inline void SyncRotation(const glm::quat _val) { rot = _val; modelMatDirty = true; lightBufferDirty = true; }
		
		
		glm::mat4 modelMat{ glm::mat4(1.0f) };

		//True if pos, rot, and/or scale have been changed but modelMat hasn't been updated yet
		bool modelMatDirty{ true };

		//True if pos and/or rot have been changed by anything other than the physics layer's jolt->ctransform sync but the ctransform->jolt sync hasn't happened yet
		//In other words, this flag gets set everytime anything other than the physics layer changes the transform, and it marks to the physics layer that the underlying jolt values have to be synced to match
		bool physicsSyncDirty{ true };
		
		//For lights
		//True if pos, rot, and/or scale have been changed but the light buffer hasn't been updated yet by RenderLayer
		bool lightBufferDirty{ true };
		
		glm::vec3 pos{ glm::vec3(0.0f) };
		glm::quat rot{ glm::quat(1.0f, 0.0f, 0.0f, 0.0f) }; //identity quaternion (wxyz: .w=1, .xyz=0)
		glm::vec3 scale{ glm::vec3(1.0f) };
	};
	
}