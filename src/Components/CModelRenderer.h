#pragma once

#include <Graphics/GPUUploader.h>


namespace NK
{

	struct CModelRenderer
	{
		friend class ModelVisibilityLayer;
		friend class RenderLayer;


	public:
		std::string modelPath;

		//Volume in world space
		glm::vec3 worldBoundaryPosition;
		glm::vec3 worldBoundaryHalfExtents;

		
	private:
		//Non-owning pointer. The RenderLayer owns and manages the GPUModel
		GPUModel* model;
		
		bool visible;
	};
	
}