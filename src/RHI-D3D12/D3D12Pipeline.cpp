#include "D3D12Pipeline.h"
#include <Core/Utils/EnumUtils.h>
#include "D3D12Shader.h"
#include "D3D12Texture.h"
#include <stdexcept>
#ifdef ERROR
	#undef ERROR
#endif
#include "D3D12RootSignature.h"

namespace NK
{
	D3D12Pipeline::D3D12Pipeline(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const PipelineDesc& _desc)
		: IPipeline(_logger, _allocator, _device, _desc)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::PIPELINE, "Initialising D3D12Pipeline\n");


		//Ensure root signature is provided
		if (_desc.rootSignature == nullptr)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::PIPELINE, "_desc.rootSignature was nullptr - a root signature is required!\n");
			throw std::runtime_error("");
		}


		switch (m_type)
		{
		case PIPELINE_TYPE::COMPUTE: CreateComputePipeline(_desc.computeShader); break;
		case PIPELINE_TYPE::GRAPHICS: CreateGraphicsPipeline(_desc.vertexShader, _desc.fragmentShader); break;
		}


		m_logger.Unindent();
	}



	D3D12Pipeline::~D3D12Pipeline()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::PIPELINE, "Shutting Down D3D12Pipeline\n");

		//ComPtrs are released automatically

		m_logger.Unindent();
	}



	void D3D12Pipeline::CreateComputePipeline(IShader* _compShader)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::PIPELINE, "Creating compute pipeline\n");

		//Create bytecode
		D3D12_SHADER_BYTECODE bytecode{};
		bytecode.pShaderBytecode = _compShader->GetBytecode().data();
		bytecode.BytecodeLength = _compShader->GetBytecode().size();

		//Create pipeline
		D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};
		desc.pRootSignature = dynamic_cast<const D3D12RootSignature* const>(m_rootSignature)->GetRootSignature();
		desc.CS = bytecode;
		D3D12_CACHED_PIPELINE_STATE cachedPSO{};
		cachedPSO.CachedBlobSizeInBytes = 0;
		cachedPSO.pCachedBlob = NULL;
		desc.CachedPSO = cachedPSO;

		//Create pipeline
		const HRESULT result{ dynamic_cast<D3D12Device&>(m_device).GetDevice()->CreateComputePipelineState(&desc, IID_PPV_ARGS(&m_pipeline)) };
		if (SUCCEEDED(result))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::PIPELINE, "Successfully created pipeline\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::PIPELINE, "Failed to create pipeline. Result = " + std::to_string(result) + "\n");
			throw std::runtime_error("");
		}


		m_logger.Unindent();
	}



	void D3D12Pipeline::CreateGraphicsPipeline(IShader* _vertShader, IShader* _fragShader)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::PIPELINE, "Creating graphics pipeline\n");


		//Create bytecodes
		D3D12_SHADER_BYTECODE vertBytecode{};
		vertBytecode.pShaderBytecode = _vertShader->GetBytecode().data();
		vertBytecode.BytecodeLength = _vertShader->GetBytecode().size();

		D3D12_SHADER_BYTECODE fragBytecode{};
		fragBytecode.pShaderBytecode = _fragShader->GetBytecode().data();
		fragBytecode.BytecodeLength = _fragShader->GetBytecode().size();

		//todo: add tessellation support


		//Define vertex input
		std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs(m_vertexInputDesc.attributeDescriptions.size());
		std::vector<std::string> semanticNamesPersistent(m_vertexInputDesc.attributeDescriptions.size()); //Needs to stay in scope
		for (std::size_t i{ 0 }; i < inputElementDescs.size(); ++i)
		{
			std::pair<std::string, UINT> semanticNameIndexPair{ GetSemanticNameIndexPair(m_vertexInputDesc.attributeDescriptions[i].attribute) };
			semanticNamesPersistent[i] = semanticNameIndexPair.first;
			inputElementDescs[i].SemanticName = semanticNamesPersistent[i].c_str();
			inputElementDescs[i].SemanticIndex = semanticNameIndexPair.second;
			inputElementDescs[i].Format = D3D12Texture::GetDXGIFormat(m_vertexInputDesc.attributeDescriptions[i].format);
			inputElementDescs[i].InputSlot = m_vertexInputDesc.attributeDescriptions[i].binding;
			inputElementDescs[i].AlignedByteOffset = m_vertexInputDesc.attributeDescriptions[i].offset;

			//Find the corresponding buffer binding to determine the input rate
			for (const VertexBufferBindingDesc& rhiBinding : m_vertexInputDesc.bufferBindingDescriptions)
			{
				if (rhiBinding.binding == m_vertexInputDesc.attributeDescriptions[i].binding)
				{
					if (rhiBinding.inputRate == VERTEX_INPUT_RATE::VERTEX)
					{
						inputElementDescs[i].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
						inputElementDescs[i].InstanceDataStepRate = 0;
					}
					else
					{
						inputElementDescs[i].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
						inputElementDescs[i].InstanceDataStepRate = 1;
					}
					break;
				}
			}
		}

		D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
		inputLayoutDesc.NumElements = static_cast<UINT>(inputElementDescs.size());
		inputLayoutDesc.pInputElementDescs = inputElementDescs.data();


		//Define rasteriser
		D3D12_RASTERIZER_DESC rasteriserDesc{};
		rasteriserDesc.FillMode = D3D12_FILL_MODE_SOLID;
		rasteriserDesc.CullMode = GetD3D12CullMode(m_rasteriserDesc.cullMode);
		rasteriserDesc.FrontCounterClockwise = m_rasteriserDesc.frontFace == WINDING_DIRECTION::COUNTER_CLOCKWISE;
		rasteriserDesc.DepthBias = m_rasteriserDesc.depthBiasEnable ? m_rasteriserDesc.depthBiasConstantFactor : 0;
		rasteriserDesc.DepthBiasClamp = m_rasteriserDesc.depthBiasClamp;
		rasteriserDesc.SlopeScaledDepthBias = m_rasteriserDesc.depthBiasSlopeFactor;
		rasteriserDesc.DepthClipEnable = true;
		rasteriserDesc.MultisampleEnable = m_multisamplingDesc.sampleCount > SAMPLE_COUNT::BIT_1;
		rasteriserDesc.AntialiasedLineEnable = false;
		rasteriserDesc.ForcedSampleCount = 0;
		rasteriserDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		//Depth-stencil
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
		depthStencilDesc.DepthEnable = m_depthStencilDesc.depthTestEnable;
		depthStencilDesc.DepthWriteMask = m_depthStencilDesc.depthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
		depthStencilDesc.DepthFunc = GetD3D12ComparisonFunc(m_depthStencilDesc.depthCompareOp);
		depthStencilDesc.StencilEnable = m_depthStencilDesc.stencilTestEnable;
		depthStencilDesc.StencilReadMask = m_depthStencilDesc.stencilReadMask;
		depthStencilDesc.StencilWriteMask = m_depthStencilDesc.stencilWriteMask;
		depthStencilDesc.FrontFace = GetD3D12DepthStencilOpDesc(m_depthStencilDesc.stencilFrontFace);
		depthStencilDesc.BackFace = GetD3D12DepthStencilOpDesc(m_depthStencilDesc.stencilBackFace);

		//Multisampling
		DXGI_SAMPLE_DESC multisamplingDesc{};
		multisamplingDesc.Count = GetD3D12SampleCount(m_multisamplingDesc.sampleCount);
		multisamplingDesc.Quality = 0;

		//Blending
		D3D12_BLEND_DESC blendDesc{};
		blendDesc.AlphaToCoverageEnable = m_multisamplingDesc.alphaToCoverageEnable;
		blendDesc.IndependentBlendEnable = (m_colourBlendDesc.attachments.size() > 1);
		for (std::size_t i{ 0 }; i < std::size(blendDesc.RenderTarget); ++i)
		{
			//Get RHI description for this specific render target and handle the case where the number of provided render target descriptions < the number of render targets
			const ColourBlendAttachmentDesc& rhiAttachment = (i < m_colourBlendDesc.attachments.size()) ? m_colourBlendDesc.attachments[i] : ColourBlendAttachmentDesc{};
			
			D3D12_RENDER_TARGET_BLEND_DESC& rtBlendDesc = blendDesc.RenderTarget[i];

			rtBlendDesc.BlendEnable = rhiAttachment.blendEnable;
			rtBlendDesc.LogicOpEnable = m_colourBlendDesc.logicOpEnable; //Apply globally to all render targets for parity with vulkan
			rtBlendDesc.SrcBlend = GetD3D12BlendFactor(rhiAttachment.srcColourBlendFactor);
			rtBlendDesc.DestBlend = GetD3D12BlendFactor(rhiAttachment.dstColourBlendFactor);
			rtBlendDesc.BlendOp = GetD3D12BlendOp(rhiAttachment.colourBlendOp);
			rtBlendDesc.SrcBlendAlpha = GetD3D12BlendFactor(rhiAttachment.srcAlphaBlendFactor);
			rtBlendDesc.DestBlendAlpha = GetD3D12BlendFactor(rhiAttachment.dstAlphaBlendFactor);
			rtBlendDesc.BlendOpAlpha = GetD3D12BlendOp(rhiAttachment.alphaBlendOp);
			rtBlendDesc.LogicOp = GetD3D12LogicOp(m_colourBlendDesc.logicOp);
			rtBlendDesc.RenderTargetWriteMask = GetD3D12RenderTargetWriteMask(rhiAttachment.colourWriteMask);
		}

		
		//Create pipeline (todo: add tessellation support)
		D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc{};
		pipelineDesc.pRootSignature = dynamic_cast<const D3D12RootSignature* const>(m_rootSignature)->GetRootSignature();
		pipelineDesc.VS = vertBytecode;
		pipelineDesc.PS = fragBytecode;
		pipelineDesc.BlendState = blendDesc;
		pipelineDesc.SampleMask = static_cast<UINT>(m_multisamplingDesc.sampleMask);
		pipelineDesc.RasterizerState = rasteriserDesc;
		pipelineDesc.DepthStencilState = depthStencilDesc;
		pipelineDesc.InputLayout = inputLayoutDesc;
		pipelineDesc.PrimitiveTopologyType = GetD3D12PrimitiveTopologyType(m_inputAssemblyDesc.topology);
		pipelineDesc.NumRenderTargets = m_colourAttachmentFormats.size();
		for (std::size_t i{ 0 }; i < m_colourAttachmentFormats.size(); ++i)
		{
			pipelineDesc.RTVFormats[i] = D3D12Texture::GetDXGIFormat(m_colourAttachmentFormats[i]);
		}
		pipelineDesc.DSVFormat = D3D12Texture::GetDXGIFormat(m_depthStencilAttachmentFormat);
		pipelineDesc.SampleDesc = multisamplingDesc;
		pipelineDesc.NodeMask = 0;
		D3D12_CACHED_PIPELINE_STATE cachedPSO{};
		cachedPSO.CachedBlobSizeInBytes = 0;
		cachedPSO.pCachedBlob = NULL;
		pipelineDesc.CachedPSO = cachedPSO;

		const HRESULT result{ dynamic_cast<D3D12Device&>(m_device).GetDevice()->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&m_pipeline)) };
		if (SUCCEEDED(result))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::PIPELINE, "Successfully created pipeline\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::PIPELINE, "Failed to create pipeline. Result = " + std::to_string(result) + "\n");
			throw std::runtime_error("");
		}


		m_logger.Unindent();
	}



	std::pair<std::string, UINT> D3D12Pipeline::GetSemanticNameIndexPair(SHADER_ATTRIBUTE _attribute)
	{
		switch (_attribute)
		{
		case SHADER_ATTRIBUTE::POSITION:	return std::make_pair<std::string, UINT>("POSITION", 0);
		case SHADER_ATTRIBUTE::NORMAL:		return std::make_pair<std::string, UINT>("NORMAL", 0);
		case SHADER_ATTRIBUTE::TEXCOORD_0:	return std::make_pair<std::string, UINT>("TEXCOORD", 0);
		case SHADER_ATTRIBUTE::COLOUR_0:	return std::make_pair<std::string, UINT>("COLOR", 0);
		default:
		{
			throw std::runtime_error("Default case reached for D3D12Pipeline::GetSemanticNameIndexPair() - attribute = " + std::to_string(std::to_underlying(_attribute)));
		}
		}
	}



	D3D12_PRIMITIVE_TOPOLOGY_TYPE D3D12Pipeline::GetD3D12PrimitiveTopologyType(INPUT_TOPOLOGY _topology)
	{
		switch (_topology)
		{
		case INPUT_TOPOLOGY::POINT_LIST:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;

		case INPUT_TOPOLOGY::LINE_LIST:
		case INPUT_TOPOLOGY::LINE_STRIP:
		case INPUT_TOPOLOGY::LINE_LIST_ADJ:
		case INPUT_TOPOLOGY::LINE_STRIP_ADJ:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

		case INPUT_TOPOLOGY::TRIANGLE_LIST:
		case INPUT_TOPOLOGY::TRIANGLE_STRIP:
		case INPUT_TOPOLOGY::TRIANGLE_LIST_ADJ:
		case INPUT_TOPOLOGY::TRIANGLE_STRIP_ADJ:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		default:
		{
			throw std::runtime_error("Default case reached for D3D12Pipeline::GetD3D12PrimitiveTopologyType() - topology = " + std::to_string(std::to_underlying(_topology)));
		}
		}
	}



	D3D12_CULL_MODE D3D12Pipeline::GetD3D12CullMode(CULL_MODE _mode)
	{
		switch (_mode)
		{
		case CULL_MODE::NONE:			return D3D12_CULL_MODE_NONE;
		case CULL_MODE::FRONT:			return D3D12_CULL_MODE_FRONT;
		case CULL_MODE::BACK:			return D3D12_CULL_MODE_BACK;
		default:
		{
			throw std::runtime_error("Default case reached for D3D12Pipeline::GetD3D12CullMode() - cull mode = " + std::to_string(std::to_underlying(_mode)));
		}
		}
	}



	D3D12_COMPARISON_FUNC D3D12Pipeline::GetD3D12ComparisonFunc(COMPARE_OP _op)
	{
		switch (_op)
		{
		case COMPARE_OP::NEVER:				return D3D12_COMPARISON_FUNC_NEVER;
		case COMPARE_OP::LESS:				return D3D12_COMPARISON_FUNC_LESS;
		case COMPARE_OP::EQUAL:				return D3D12_COMPARISON_FUNC_EQUAL;
		case COMPARE_OP::LESS_OR_EQUAL:		return D3D12_COMPARISON_FUNC_LESS_EQUAL;
		case COMPARE_OP::GREATER:			return D3D12_COMPARISON_FUNC_GREATER;
		case COMPARE_OP::NOT_EQUAL:			return D3D12_COMPARISON_FUNC_NOT_EQUAL;
		case COMPARE_OP::GREATER_OR_EQUAL:	return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		case COMPARE_OP::ALWAYS:			return D3D12_COMPARISON_FUNC_ALWAYS;
		default:
		{
			throw std::runtime_error("Default case reached for D3D12Pipeline::GetD3D12ComparisonFunc() - op = " + std::to_string(std::to_underlying(_op)));
		}
		}
	}



	D3D12_STENCIL_OP D3D12Pipeline::GetD3D12StencilOp(STENCIL_OP _op)
	{
		switch (_op)
		{
		case STENCIL_OP::KEEP:					return D3D12_STENCIL_OP_KEEP;
		case STENCIL_OP::ZERO:					return D3D12_STENCIL_OP_ZERO;
		case STENCIL_OP::REPLACE:				return D3D12_STENCIL_OP_REPLACE;
		case STENCIL_OP::INCREMENT_AND_CLAMP:	return D3D12_STENCIL_OP_INCR_SAT;
		case STENCIL_OP::DECREMENT_AND_CLAMP:	return D3D12_STENCIL_OP_DECR_SAT;
		case STENCIL_OP::INVERT:				return D3D12_STENCIL_OP_INVERT;
		case STENCIL_OP::INCREMENT_AND_WRAP:	return D3D12_STENCIL_OP_INCR;
		case STENCIL_OP::DECREMENT_AND_WRAP:	return D3D12_STENCIL_OP_DECR;
		default:
		{
			throw std::runtime_error("Default case reached for D3D12Pipeline::GetD3D12StencilOp() - op = " + std::to_string(std::to_underlying(_op)));
		}
		}
	}



	D3D12_DEPTH_STENCILOP_DESC D3D12Pipeline::GetD3D12DepthStencilOpDesc(StencilOpState _state)
	{
		D3D12_DEPTH_STENCILOP_DESC d3d12Desc{};
		d3d12Desc.StencilFailOp = GetD3D12StencilOp(_state.failOp);
		d3d12Desc.StencilDepthFailOp = GetD3D12StencilOp(_state.depthFailOp);
		d3d12Desc.StencilPassOp = GetD3D12StencilOp(_state.passOp);
		return d3d12Desc;
	}



	UINT D3D12Pipeline::GetD3D12SampleCount(SAMPLE_COUNT _count)
	{
		switch (_count)
		{
		case SAMPLE_COUNT::BIT_1:	return 1;
		case SAMPLE_COUNT::BIT_2:	return 2;
		case SAMPLE_COUNT::BIT_4:	return 4;
		case SAMPLE_COUNT::BIT_8:	return 8;
		case SAMPLE_COUNT::BIT_16:	return 16;
		case SAMPLE_COUNT::BIT_32:	return 32;
		default:
		{
			throw std::runtime_error("Default case reached for D3D12Pipeline::GetD3D12SampleCount() - sample count = " + std::to_string(std::to_underlying(_count)));
		}
		}
	}



	UINT8 D3D12Pipeline::GetD3D12RenderTargetWriteMask(COLOUR_ASPECT_FLAGS _mask)
	{
		UINT8 result{ 0 };
		if (EnumUtils::Contains(_mask, COLOUR_ASPECT_FLAGS::R_BIT)) { result |= D3D12_COLOR_WRITE_ENABLE_RED; }
		if (EnumUtils::Contains(_mask, COLOUR_ASPECT_FLAGS::G_BIT)) { result |= D3D12_COLOR_WRITE_ENABLE_GREEN; }
		if (EnumUtils::Contains(_mask, COLOUR_ASPECT_FLAGS::B_BIT)) { result |= D3D12_COLOR_WRITE_ENABLE_BLUE; }
		if (EnumUtils::Contains(_mask, COLOUR_ASPECT_FLAGS::A_BIT)) { result |= D3D12_COLOR_WRITE_ENABLE_ALPHA; }
		return result;
	}



	D3D12_BLEND D3D12Pipeline::GetD3D12BlendFactor(BLEND_FACTOR _factor)
	{
		switch (_factor)
		{
		case BLEND_FACTOR::ZERO:                     return D3D12_BLEND_ZERO;
		case BLEND_FACTOR::ONE:                      return D3D12_BLEND_ONE;
		case BLEND_FACTOR::SRC_COLOR:                return D3D12_BLEND_SRC_COLOR;
		case BLEND_FACTOR::ONE_MINUS_SRC_COLOR:      return D3D12_BLEND_INV_SRC_COLOR;
		case BLEND_FACTOR::DST_COLOR:                return D3D12_BLEND_DEST_COLOR;
		case BLEND_FACTOR::ONE_MINUS_DST_COLOR:      return D3D12_BLEND_INV_DEST_COLOR;
		case BLEND_FACTOR::SRC_ALPHA:                return D3D12_BLEND_SRC_ALPHA;
		case BLEND_FACTOR::ONE_MINUS_SRC_ALPHA:      return D3D12_BLEND_INV_SRC_ALPHA;
		case BLEND_FACTOR::DST_ALPHA:                return D3D12_BLEND_DEST_ALPHA;
		case BLEND_FACTOR::ONE_MINUS_DST_ALPHA:      return D3D12_BLEND_INV_DEST_ALPHA;
		case BLEND_FACTOR::CONSTANT_COLOR:           return D3D12_BLEND_BLEND_FACTOR;
		case BLEND_FACTOR::ONE_MINUS_CONSTANT_COLOR: return D3D12_BLEND_INV_BLEND_FACTOR;
		case BLEND_FACTOR::CONSTANT_ALPHA:           return D3D12_BLEND_ALPHA_FACTOR;
		case BLEND_FACTOR::ONE_MINUS_CONSTANT_ALPHA: return D3D12_BLEND_INV_ALPHA_FACTOR;
		case BLEND_FACTOR::SRC_ALPHA_SATURATE:       return D3D12_BLEND_SRC_ALPHA_SAT;
		case BLEND_FACTOR::SRC1_COLOR:               return D3D12_BLEND_SRC1_COLOR;
		case BLEND_FACTOR::ONE_MINUS_SRC1_COLOR:     return D3D12_BLEND_INV_SRC1_COLOR;
		case BLEND_FACTOR::SRC1_ALPHA:               return D3D12_BLEND_SRC1_ALPHA;
		case BLEND_FACTOR::ONE_MINUS_SRC1_ALPHA:     return D3D12_BLEND_INV_SRC1_ALPHA;
		default:
		{
			throw std::runtime_error("Default case reached for D3D12Pipeline::GetD3D12BlendFactor() - factor = " + std::to_string(std::to_underlying(_factor)));
		}
		}
	}


	D3D12_BLEND_OP D3D12Pipeline::GetD3D12BlendOp(BLEND_OP _op)
	{
		switch (_op)
		{
		case BLEND_OP::ADD:              return D3D12_BLEND_OP_ADD;
		case BLEND_OP::SUBTRACT:         return D3D12_BLEND_OP_SUBTRACT;
		case BLEND_OP::REVERSE_SUBTRACT: return D3D12_BLEND_OP_REV_SUBTRACT;
		case BLEND_OP::MIN:              return D3D12_BLEND_OP_MIN;
		case BLEND_OP::MAX:              return D3D12_BLEND_OP_MAX;
		default:
		{
			throw std::runtime_error("Default case reached for D3D12Pipeline::GetD3D12BlendOp() - op = " + std::to_string(std::to_underlying(_op)));
		}
		}
	}



	D3D12_LOGIC_OP D3D12Pipeline::GetD3D12LogicOp(LOGIC_OP _op)
	{
		switch (_op)
		{
		case LOGIC_OP::CLEAR:          return D3D12_LOGIC_OP_CLEAR;
		case LOGIC_OP::AND:            return D3D12_LOGIC_OP_AND;
		case LOGIC_OP::AND_REVERSE:    return D3D12_LOGIC_OP_AND_REVERSE;
		case LOGIC_OP::COPY:           return D3D12_LOGIC_OP_COPY;
		case LOGIC_OP::AND_INVERTED:   return D3D12_LOGIC_OP_AND_INVERTED;
		case LOGIC_OP::NO_OP:          return D3D12_LOGIC_OP_NOOP;
		case LOGIC_OP::XOR:            return D3D12_LOGIC_OP_XOR;
		case LOGIC_OP::OR:             return D3D12_LOGIC_OP_OR;
		case LOGIC_OP::NOR:            return D3D12_LOGIC_OP_NOR;
		case LOGIC_OP::EQUIVALENT:     return D3D12_LOGIC_OP_EQUIV;
		case LOGIC_OP::INVERT:         return D3D12_LOGIC_OP_INVERT;
		case LOGIC_OP::OR_REVERSE:     return D3D12_LOGIC_OP_OR_REVERSE;
		case LOGIC_OP::COPY_INVERTED:  return D3D12_LOGIC_OP_COPY_INVERTED;
		case LOGIC_OP::OR_INVERTED:    return D3D12_LOGIC_OP_OR_INVERTED;
		case LOGIC_OP::NAND:           return D3D12_LOGIC_OP_NAND;
		case LOGIC_OP::SET:            return D3D12_LOGIC_OP_SET;
		default:
		{
			throw std::runtime_error("Default case reached for D3D12Pipeline::GetD3D12LogicOp() - op = " + std::to_string(std::to_underlying(_op)));
		}
		}
	}

}