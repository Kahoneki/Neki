#pragma once

#include "IRootSignature.h"
#include "IShader.h"

#include <Core/Memory/IAllocator.h>
#include <Types/NekiTypes.h>


namespace NK
{

	struct PipelineDesc
	{
		PIPELINE_TYPE type;

		IShader* computeShader;
		IShader* vertexShader;
		IShader* fragmentShader;

		IRootSignature* rootSignature;
		
		VertexInputDesc vertexInputDesc;
		InputAssemblyDesc inputAssemblyDesc;
		RasteriserDesc rasteriserDesc;
		DepthStencilDesc depthStencilDesc;
		MultisamplingDesc multisamplingDesc;
		ColourBlendDesc colourBlendDesc;

		std::vector<DATA_FORMAT> colourAttachmentFormats;
		DATA_FORMAT depthStencilAttachmentFormat;
	};


	class IPipeline
	{
	public:
		virtual ~IPipeline() = default;

		[[nodiscard]] inline const IRootSignature* GetRootSignature() const { return m_rootSignature; }


	protected:
		IPipeline(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const PipelineDesc& _desc)
		: m_logger(_logger), m_allocator(_allocator), m_device(_device),
		  m_type(_desc.type), m_rootSignature(_desc.rootSignature),
		  m_vertexInputDesc(_desc.vertexInputDesc), m_inputAssemblyDesc(_desc.inputAssemblyDesc), m_rasteriserDesc(_desc.rasteriserDesc), m_depthStencilDesc(_desc.depthStencilDesc), m_multisamplingDesc(_desc.multisamplingDesc), m_colourBlendDesc(_desc.colourBlendDesc),
		  m_colourAttachmentFormats(_desc.colourAttachmentFormats), m_depthStencilAttachmentFormat(_desc.depthStencilAttachmentFormat) {}

		
		//Dependency injections
		ILogger& m_logger;
		IAllocator& m_allocator;
		IDevice& m_device;

		PIPELINE_TYPE m_type;

		const IRootSignature* const m_rootSignature;
		
		VertexInputDesc m_vertexInputDesc;
		InputAssemblyDesc m_inputAssemblyDesc;
		RasteriserDesc m_rasteriserDesc;
		DepthStencilDesc m_depthStencilDesc;
		MultisamplingDesc m_multisamplingDesc;
		ColourBlendDesc m_colourBlendDesc;
		
		std::vector<DATA_FORMAT> m_colourAttachmentFormats;
		DATA_FORMAT m_depthStencilAttachmentFormat;
	};
	
}