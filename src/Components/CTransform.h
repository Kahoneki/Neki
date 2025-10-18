#pragma once

#include <glm/glm.hpp>


namespace NK
{

	struct CTransform
	{
		friend class RenderSystem;


	public:
		[[nodiscard]] inline glm::vec3 GetPosition() const { return pos; }
		[[nodiscard]] inline glm::vec3 GetRotation() const { return rot; }
		[[nodiscard]] inline glm::vec3 GetScale() const { return scale; }

		inline void SetPosition(const glm::vec3 _val) { pos = _val; posDirty = true; }
		//Set euler rotation in radians
		inline void SetRotation(const glm::vec3 _val) { rot = _val; rotDirty = true; }
		inline void SetScale(const glm::vec3 _val) { scale = _val; scaleDirty = true; }
		
		
	private:
		glm::mat4 modelMat{ glm::mat4(1.0f) };

		//True if pos, rot, and/or scale have been changed but modelMat hasn't been updated yet
		bool posDirty{ false };
		bool rotDirty{ false };
		bool scaleDirty{ false };
		
		glm::vec3 pos{ glm::vec3(0.0f) };
		glm::vec3 rot{ glm::vec3(0.0f) }; //Euler in radians
		glm::vec3 scale{ glm::vec3(0.0f) };

		//Old values since last time model matrix was updated
		glm::vec3 oldPos{ glm::vec3(0.0f) };
		glm::vec3 oldRot{ glm::vec3(0.0f) }; //Euler in radians
		glm::vec3 oldScale{ glm::vec3(0.0f) };
	};
	
}