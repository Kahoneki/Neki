#pragma once

#include <Graphics/GPUUploader.h>


namespace NK
{

	struct CModelRenderer final
	{
		friend class ModelVisibilityLayer;
		friend class RenderLayer;


	public:
		std::string modelPath;

		//Volume in local space
		glm::vec3 localSpaceOrigin;
		glm::vec3 localSpaceHalfExtents;

		
	private:
		//Non-owning pointer. The RenderLayer owns and manages the GPUModel
		GPUModel* model;
		
		bool visible{ true };
	};
	
}