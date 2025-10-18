#pragma once
#include <Graphics/GPUUploader.h>
#include <RHI/IBufferView.h>


namespace NK
{

	struct CModelRenderer
	{
		friend class RenderSystem;


	public:
		std::string modelPath;


	private:
		//Non-owning pointer. The RenderSystem owns and manages the GPUModel
		GPUModel* model;
	};
	
}