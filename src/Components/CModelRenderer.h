#pragma once
#include <Graphics/GPUUploader.h>
#include <RHI/IBufferView.h>


namespace NK
{

	struct CModelRenderer
	{
		friend class RenderLayer;


	public:
		std::string modelPath;


	private:
		//Non-owning pointer. The RenderLayer owns and manages the GPUModel
		GPUModel* model;
	};
	
}