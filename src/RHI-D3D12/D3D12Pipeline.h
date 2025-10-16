#pragma once

#include "D3D12Device.h"

#include <RHI/IPipeline.h>


namespace NK
{
	
	class D3D12Pipeline final : public IPipeline
	{
	public:
		D3D12Pipeline(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const PipelineDesc& _desc);
		virtual ~D3D12Pipeline() override;

		//D3D12 internal API (for use by other RHI-D3D12 classes)
		[[nodiscard]] inline ID3D12PipelineState* GetPipeline() { return m_pipeline.Get(); }

		[[nodiscard]] static std::pair<std::string, UINT> GetSemanticNameIndexPair(SHADER_ATTRIBUTE _attribute);
		[[nodiscard]] static D3D12_CULL_MODE GetD3D12CullMode(CULL_MODE _mode);
		[[nodiscard]] static D3D12_PRIMITIVE_TOPOLOGY_TYPE GetD3D12PrimitiveTopologyType(INPUT_TOPOLOGY _topology);
		[[nodiscard]] static D3D12_COMPARISON_FUNC GetD3D12ComparisonFunc(COMPARE_OP _op);
		[[nodiscard]] static D3D12_STENCIL_OP GetD3D12StencilOp(STENCIL_OP _op);
		[[nodiscard]] static D3D12_DEPTH_STENCILOP_DESC GetD3D12DepthStencilOpDesc(StencilOpState _state);
		[[nodiscard]] static UINT GetD3D12SampleCount(SAMPLE_COUNT _count);
		[[nodiscard]] static UINT8 GetD3D12RenderTargetWriteMask(COLOUR_ASPECT_FLAGS _mask);
		[[nodiscard]] static D3D12_BLEND GetD3D12BlendFactor(BLEND_FACTOR _factor);
		[[nodiscard]] static D3D12_BLEND_OP GetD3D12BlendOp(BLEND_OP _op);
		[[nodiscard]] static D3D12_LOGIC_OP GetD3D12LogicOp(LOGIC_OP _op);


	private:
		void CreateComputePipeline(IShader* _compShader);
		void CreateGraphicsPipeline(IShader* _vertShader, IShader* _fragShader);


		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipeline;
	};

}