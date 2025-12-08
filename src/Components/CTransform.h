#pragma once

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>


namespace NK
{

	struct CTransform final
	{
		friend class RenderLayer;


	public:
		[[nodiscard]] inline glm::vec3 GetPosition() const { return pos; }
		[[nodiscard]] inline glm::vec3 GetRotation() const { return rot; }
		[[nodiscard]] inline glm::vec3 GetScale() const { return scale; }
		[[nodiscard]] inline glm::mat4 GetModelMatrix()
		{
			if (!modelMatDirty) { return modelMat; }
			//Scale -> Rotation -> Translation
			//Because matrix multiplication order is reversed, do trans * rot * scale

			const glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scale);
			const glm::mat4 rotMat = glm::mat4_cast(glm::quat(rot));
			const glm::mat4 transMat = glm::translate(glm::mat4(1.0f), pos);

			modelMat = transMat * rotMat * scaleMat;

			modelMatDirty = false;

			return modelMat;
		}

		inline void SetPosition(const glm::vec3 _val) { pos = _val; modelMatDirty = true; lightBufferDirty = true; }
		//Set euler rotation in radians
		inline void SetRotation(const glm::vec3 _val) { rot = _val; modelMatDirty = true; lightBufferDirty = true; }
		inline void SetScale(const glm::vec3 _val) { scale = _val; modelMatDirty = true; lightBufferDirty = true; }
		
		
	private:
		glm::mat4 modelMat{ glm::mat4(1.0f) };

		//True if pos, rot, and/or scale have been changed but modelMat hasn't been updated yet
		bool modelMatDirty{ true };

		//For lights
		//True if pos, rot, and/or scale have been changed but the light buffer hasn't been updated yet by RenderLayer
		bool lightBufferDirty{ true };
		
		glm::vec3 pos{ glm::vec3(0.0f) };
		glm::vec3 rot{ glm::vec3(0.0f) }; //Euler in radians
		glm::vec3 scale{ glm::vec3(1.0f) };
	};
	
}