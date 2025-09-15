#include "VulkanPipeline.h"
#include <stdexcept>
#include <Core/Utils/EnumUtils.h>
#include "VulkanDevice.h"
#include "VulkanShader.h"
#include "VulkanTexture.h"

namespace NK
{
	VulkanPipeline::VulkanPipeline(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const PipelineDesc& _desc)
	: IPipeline(_logger, _allocator, _device, _desc)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::PIPELINE, "Initialising VulkanPipeline\n");


		CreateShaderModules(_desc.computeShader, _desc.vertexShader, _desc.fragmentShader);
		switch (m_type)
		{
		case PIPELINE_TYPE::COMPUTE: CreateComputePipeline(); break;
		case PIPELINE_TYPE::GRAPHICS: CreateGraphicsPipeline(); break;
		}


		m_logger.Unindent();
	}



	VulkanPipeline::~VulkanPipeline()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::PIPELINE, "Shutting Down VulkanPipeline\n");


		if (m_pipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), m_pipeline, m_allocator.GetVulkanCallbacks());
			m_pipeline = VK_NULL_HANDLE;
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::PIPELINE, "Pipeline Destroyed\n");
		}

		if (m_computeShaderModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), m_computeShaderModule, m_allocator.GetVulkanCallbacks());
			m_computeShaderModule = VK_NULL_HANDLE;
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::PIPELINE, "Compute Shader Module Destroyed\n");
		}

		if (m_vertexShaderModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), m_vertexShaderModule, m_allocator.GetVulkanCallbacks());
			m_vertexShaderModule = VK_NULL_HANDLE;
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::PIPELINE, "Vertex Shader Module Destroyed\n");
		}

		if (m_fragmentShaderModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), m_fragmentShaderModule, m_allocator.GetVulkanCallbacks());
			m_fragmentShaderModule = VK_NULL_HANDLE;
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::PIPELINE, "Fragment Shader Module Destroyed\n");
		}


		m_logger.Unindent();
	}



	void VulkanPipeline::CreateShaderModules(IShader* _compute, IShader* _vertex, IShader* _fragment)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::PIPELINE, "Creating shader modules\n");


		switch (m_type)
		{

		case PIPELINE_TYPE::COMPUTE:
		{
			if (_compute == nullptr)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::PIPELINE, "_desc.type = PIPELINE_TYPE::COMPUTE but _desc.computeShader = nullptr\n");
				throw std::runtime_error("");
			}
			m_computeShaderModule = CreateShaderModule(_compute);
			break;
		}

		case PIPELINE_TYPE::GRAPHICS:
		{
			if (_vertex == nullptr)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::PIPELINE, "_desc.type = PIPELINE_TYPE::GRAPHICS but _desc.vertexShader = nullptr\n");
				throw std::runtime_error("");
			}
			if (_fragment == nullptr)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::PIPELINE, "_desc.type = PIPELINE_TYPE::GRAPHICS but _desc.fragmentShader = nullptr\n");
				throw std::runtime_error("");
			}
			m_vertexShaderModule = CreateShaderModule(_vertex);
			m_fragmentShaderModule = CreateShaderModule(_fragment);
			break;
		}

		}


		m_logger.Unindent();
	}



	void VulkanPipeline::CreateComputePipeline()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::PIPELINE, "Creating compute pipeline\n");
		

		//Create shader stage
		VkPipelineShaderStageCreateInfo compShaderStageInfo{};
		compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		compShaderStageInfo.module = m_computeShaderModule;
		compShaderStageInfo.pName = "CSMain";

		//Create pipeline
		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.stage = compShaderStageInfo;
		pipelineInfo.layout = dynamic_cast<VulkanDevice&>(m_device).GetPipelineLayout();
		const VkResult result{ vkCreateComputePipelines(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, m_allocator.GetVulkanCallbacks(), &m_pipeline) };
		if (result == VK_SUCCESS)
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



	void VulkanPipeline::CreateGraphicsPipeline()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::PIPELINE, "Creating graphics pipeline\n");


		//Create shader stages
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = m_vertexShaderModule;
		vertShaderStageInfo.pName = "VSMain";
		shaderStages.push_back(vertShaderStageInfo);

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = m_fragmentShaderModule;
		fragShaderStageInfo.pName = "FSMain";
		shaderStages.push_back(fragShaderStageInfo);

		//todo: add tessellation support


		//Define vertex input
		std::vector<VkVertexInputBindingDescription> bindingDescs(m_vertexInputDesc.bufferBindingDescriptions.size());
		for (std::size_t i{ 0 }; i < bindingDescs.size(); ++i)
		{
			bindingDescs[i].binding = m_vertexInputDesc.bufferBindingDescriptions[i].binding;
			bindingDescs[i].stride = m_vertexInputDesc.bufferBindingDescriptions[i].stride;
			switch (m_vertexInputDesc.bufferBindingDescriptions[i].inputRate)
			{
			case VERTEX_INPUT_RATE::VERTEX: bindingDescs[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX; break;
			case VERTEX_INPUT_RATE::INSTANCE: bindingDescs[i].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE; break;
			}
		}

		std::vector<VkVertexInputAttributeDescription> attributeDescs(m_vertexInputDesc.attributeDescriptions.size());
		for (std::size_t i{ 0 }; i < attributeDescs.size(); ++i)
		{
			attributeDescs[i].location = std::to_underlying(m_vertexInputDesc.attributeDescriptions[i].attribute);
			attributeDescs[i].binding = m_vertexInputDesc.attributeDescriptions[i].binding;
			attributeDescs[i].format = VulkanTexture::GetVulkanFormat(m_vertexInputDesc.attributeDescriptions[i].format);
			attributeDescs[i].offset = m_vertexInputDesc.attributeDescriptions[i].offset;
		}

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = bindingDescs.size();
		vertexInputInfo.pVertexBindingDescriptions = bindingDescs.data();
		vertexInputInfo.vertexAttributeDescriptionCount = attributeDescs.size();
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescs.data();


		//Define other static properties

		//Input assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
		inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyInfo.topology = GetVulkanTopology(m_inputAssemblyDesc.topology);
		inputAssemblyInfo.primitiveRestartEnable = VK_FALSE; //For parity with dx12

		//Viewport and scissor
		VkPipelineViewportStateCreateInfo viewportStateInfo{};
		viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateInfo.viewportCount = 1;
		viewportStateInfo.scissorCount = 1;

		//Rasteriser
		VkPipelineRasterizationStateCreateInfo rasteriserInfo{};
		rasteriserInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasteriserInfo.depthClampEnable = VK_FALSE;
		rasteriserInfo.rasterizerDiscardEnable = VK_FALSE;
		rasteriserInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasteriserInfo.cullMode = GetVulkanCullMode(m_rasteriserDesc.cullMode);
		rasteriserInfo.frontFace = GetVulkanWindingDirection(m_rasteriserDesc.frontFace);
		rasteriserInfo.depthBiasEnable = m_rasteriserDesc.depthBiasEnable;
		rasteriserInfo.depthBiasConstantFactor = m_rasteriserDesc.depthBiasConstantFactor;
		rasteriserInfo.depthBiasClamp = m_rasteriserDesc.depthBiasClamp;
		rasteriserInfo.depthBiasSlopeFactor = m_rasteriserDesc.depthBiasSlopeFactor;
		rasteriserInfo.lineWidth = 1.0f; //For parity with dx12

		//Depth-stencil
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
		depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilInfo.depthTestEnable = m_depthStencilDesc.depthTestEnable;
		depthStencilInfo.depthWriteEnable = m_depthStencilDesc.depthWriteEnable;
		depthStencilInfo.depthCompareOp = GetVulkanCompareOp(m_depthStencilDesc.depthCompareOp);
		depthStencilInfo.depthBoundsTestEnable = VK_FALSE; //For parity with dx12
		depthStencilInfo.stencilTestEnable = m_depthStencilDesc.stencilTestEnable;
		depthStencilInfo.front = GetVulkanStencilOpState(m_depthStencilDesc.stencilFrontFace, m_depthStencilDesc.stencilReadMask, m_depthStencilDesc.stencilWriteMask);
		depthStencilInfo.back = GetVulkanStencilOpState(m_depthStencilDesc.stencilBackFace, m_depthStencilDesc.stencilReadMask, m_depthStencilDesc.stencilWriteMask);

		//Multisampling
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = false; //For parity with dx12
		multisampling.rasterizationSamples = GetVulkanSampleCount(m_multisamplingDesc.sampleCount);
		multisampling.minSampleShading = false; //For parity with dx12
		multisampling.pSampleMask = &m_multisamplingDesc.sampleMask;
		multisampling.alphaToCoverageEnable = m_multisamplingDesc.alphaToCoverageEnable;
		multisampling.alphaToOneEnable = false; //For parity with dx12
		
		//Blending
		std::vector<VkPipelineColorBlendAttachmentState> vulkanColourBlendAttachments(m_colourBlendDesc.attachments.size());
		for (std::size_t i{ 0 }; i < m_colourBlendDesc.attachments.size(); ++i)
		{
			vulkanColourBlendAttachments[i].colorWriteMask = GetVulkanColourAspectFlags(m_colourBlendDesc.attachments[i].colourWriteMask);
			vulkanColourBlendAttachments[i].blendEnable = m_colourBlendDesc.attachments[i].blendEnable;
			vulkanColourBlendAttachments[i].srcColorBlendFactor = GetVulkanBlendFactor(m_colourBlendDesc.attachments[i].srcColourBlendFactor);
			vulkanColourBlendAttachments[i].dstColorBlendFactor = GetVulkanBlendFactor(m_colourBlendDesc.attachments[i].dstColourBlendFactor);
			vulkanColourBlendAttachments[i].colorBlendOp = GetVulkanBlendOp(m_colourBlendDesc.attachments[i].colourBlendOp);
			vulkanColourBlendAttachments[i].srcAlphaBlendFactor = GetVulkanBlendFactor(m_colourBlendDesc.attachments[i].srcAlphaBlendFactor);
			vulkanColourBlendAttachments[i].dstAlphaBlendFactor = GetVulkanBlendFactor(m_colourBlendDesc.attachments[i].dstAlphaBlendFactor);
			vulkanColourBlendAttachments[i].alphaBlendOp = GetVulkanBlendOp(m_colourBlendDesc.attachments[i].alphaBlendOp);
		}
		VkPipelineColorBlendStateCreateInfo colourBlending{};
		colourBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colourBlending.logicOpEnable = m_colourBlendDesc.logicOpEnable;
		colourBlending.logicOp = GetVulkanLogicOp(m_colourBlendDesc.logicOp);
		colourBlending.attachmentCount = vulkanColourBlendAttachments.size();
		colourBlending.pAttachments = vulkanColourBlendAttachments.data();


		//Define dynamic states
		VkDynamicState dynamicStates[]
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
			VK_DYNAMIC_STATE_BLEND_CONSTANTS,
			VK_DYNAMIC_STATE_STENCIL_REFERENCE,
		};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<std::uint32_t>(std::size(dynamicStates));
		dynamicState.pDynamicStates = dynamicStates;


		//Define dynamic rendering
		VkPipelineRenderingCreateInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		renderingInfo.colorAttachmentCount = static_cast<std::uint32_t>(m_colourAttachmentFormats.size());
		std::vector<VkFormat> vulkanColourAttachmentFormats(m_colourAttachmentFormats.size());
		for (DATA_FORMAT f : m_colourAttachmentFormats)
		{
			vulkanColourAttachmentFormats.push_back(VulkanTexture::GetVulkanFormat(f));
		}
		renderingInfo.pColorAttachmentFormats = vulkanColourAttachmentFormats.data();
		//Use same format for depth and stencil for parity with dx12
		renderingInfo.depthAttachmentFormat = VulkanTexture::GetVulkanFormat(m_depthStencilAttachmentFormat);
		renderingInfo.stencilAttachmentFormat = VulkanTexture::GetVulkanFormat(m_depthStencilAttachmentFormat);


		//Create pipeline (todo: add tessellation support)
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.pNext = &renderingInfo;
		pipelineInfo.flags = 0;
		pipelineInfo.stageCount = shaderStages.size();
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
		pipelineInfo.pViewportState = &viewportStateInfo;
		pipelineInfo.pRasterizationState = &rasteriserInfo;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencilInfo;
		pipelineInfo.pColorBlendState = &colourBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = dynamic_cast<VulkanDevice&>(m_device).GetPipelineLayout();
		const VkResult result{ vkCreateGraphicsPipelines(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, m_allocator.GetVulkanCallbacks(), &m_pipeline) };
		if (result == VK_SUCCESS)
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



	VkShaderModule VulkanPipeline::CreateShaderModule(IShader* _shader) const
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = _shader->GetBytecode().size();
		createInfo.pCode = reinterpret_cast<const std::uint32_t*>(_shader->GetBytecode().data());

		VkShaderModule module;
		const VkResult result{ vkCreateShaderModule(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), &createInfo, m_allocator.GetVulkanCallbacks(), &module) };
		if (result != VK_SUCCESS)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::PIPELINE, "Failed to create shader module. Result = " + std::to_string(result) + "\n");
			throw std::runtime_error("");
		}

		return module;
	}



	VkPrimitiveTopology VulkanPipeline::GetVulkanTopology(INPUT_TOPOLOGY _topology)
	{
		switch (_topology)
		{
		case INPUT_TOPOLOGY::POINT_LIST:			return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		case INPUT_TOPOLOGY::LINE_LIST:				return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		case INPUT_TOPOLOGY::LINE_STRIP:			return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		case INPUT_TOPOLOGY::TRIANGLE_LIST:			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		case INPUT_TOPOLOGY::TRIANGLE_STRIP:		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

		case INPUT_TOPOLOGY::LINE_LIST_ADJ:			return VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
		case INPUT_TOPOLOGY::LINE_STRIP_ADJ:		return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
		case INPUT_TOPOLOGY::TRIANGLE_LIST_ADJ:		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
		case INPUT_TOPOLOGY::TRIANGLE_STRIP_ADJ:	return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;

		default:
		{
			throw std::runtime_error("Default case reached for VulkanPipeline::GetVulkanTopology() - topology = " + std::to_string(std::to_underlying(_topology)));
		}
		}
	}



	VkCullModeFlags VulkanPipeline::GetVulkanCullMode(CULL_MODE _mode)
	{
		switch (_mode)
		{
		case CULL_MODE::NONE:			return VK_CULL_MODE_NONE;
		case CULL_MODE::FRONT:			return VK_CULL_MODE_FRONT_BIT;
		case CULL_MODE::BACK:			return VK_CULL_MODE_BACK_BIT;
		default:
		{
			throw std::runtime_error("Default case reached for VulkanPipeline::GetVulkanCullMode() - cull mode = " + std::to_string(std::to_underlying(_mode)));
		}
		}
	}



	VkFrontFace VulkanPipeline::GetVulkanWindingDirection(WINDING_DIRECTION _direction)
	{
		switch (_direction)
		{
		case WINDING_DIRECTION::CLOCKWISE:			return VK_FRONT_FACE_CLOCKWISE;
		case WINDING_DIRECTION::COUNTER_CLOCKWISE:	return VK_FRONT_FACE_COUNTER_CLOCKWISE;
		default:
		{
			throw std::runtime_error("Default case reached for VulkanPipeline::GetVulkanWindingDirection() - winding direction = " + std::to_string(std::to_underlying(_direction)));
		}
		}
	}



	VkCompareOp VulkanPipeline::GetVulkanCompareOp(COMPARE_OP _op)
	{
		switch (_op)
		{
		case COMPARE_OP::NEVER:				return VK_COMPARE_OP_NEVER;
		case COMPARE_OP::LESS:				return VK_COMPARE_OP_LESS;
		case COMPARE_OP::EQUAL:				return VK_COMPARE_OP_EQUAL;
		case COMPARE_OP::LESS_OR_EQUAL:		return VK_COMPARE_OP_LESS_OR_EQUAL;
		case COMPARE_OP::GREATER:			return VK_COMPARE_OP_GREATER;
		case COMPARE_OP::NOT_EQUAL:			return VK_COMPARE_OP_NOT_EQUAL;
		case COMPARE_OP::GREATER_OR_EQUAL:	return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case COMPARE_OP::ALWAYS:			return VK_COMPARE_OP_ALWAYS;
		default:
		{
			throw std::runtime_error("Default case reached for VulkanPipeline::GetVulkanCompareOp() - op = " + std::to_string(std::to_underlying(_op)));
		}
		}
	}



	VkStencilOp VulkanPipeline::GetVulkanStencilOp(STENCIL_OP _op)
	{
		switch (_op)
		{
		case STENCIL_OP::KEEP:					return VK_STENCIL_OP_KEEP;
		case STENCIL_OP::ZERO:					return VK_STENCIL_OP_ZERO;
		case STENCIL_OP::REPLACE:				return VK_STENCIL_OP_REPLACE;
		case STENCIL_OP::INCREMENT_AND_CLAMP:	return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
		case STENCIL_OP::DECREMENT_AND_CLAMP:	return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
		case STENCIL_OP::INVERT:				return VK_STENCIL_OP_INVERT;
		case STENCIL_OP::INCREMENT_AND_WRAP:	return VK_STENCIL_OP_INCREMENT_AND_WRAP;
		case STENCIL_OP::DECREMENT_AND_WRAP:	return VK_STENCIL_OP_DECREMENT_AND_WRAP;
		default:
		{
			throw std::runtime_error("Default case reached for VulkanPipeline::GetVulkanStencilOp() - op = " + std::to_string(std::to_underlying(_op)));
		}
		}
	}



	std::uint32_t VulkanPipeline::Convert8BitMaskTo32BitMask(std::uint8_t _mask)
	{
		//For each set bit in the input mask, the corresponding 4 bits in the output mask will be set
		std::uint32_t result{ 0 };
		for (int i{ 0 }; i < 8; ++i)
		{
			if ((_mask >> i) & 1)
			{
				result |= (0b1111 << (i * 4));
			}
		}
		return result;
	}



	VkStencilOpState VulkanPipeline::GetVulkanStencilOpState(StencilOpState _state, std::uint8_t _readMask, std::uint8_t _writeMask)
	{
		VkStencilOpState vkState{};
		vkState.failOp = GetVulkanStencilOp(_state.failOp);
		vkState.passOp = GetVulkanStencilOp(_state.passOp);
		vkState.depthFailOp = GetVulkanStencilOp(_state.depthFailOp);
		vkState.compareOp = GetVulkanCompareOp(_state.compareOp);
		vkState.compareMask = Convert8BitMaskTo32BitMask(_readMask);
		vkState.writeMask = Convert8BitMaskTo32BitMask(_writeMask);
		return vkState;
	}



	VkSampleCountFlagBits VulkanPipeline::GetVulkanSampleCount(SAMPLE_COUNT _count)
	{
		switch (_count)
		{
		case SAMPLE_COUNT::BIT_1:	return VK_SAMPLE_COUNT_1_BIT;
		case SAMPLE_COUNT::BIT_2:	return VK_SAMPLE_COUNT_2_BIT;
		case SAMPLE_COUNT::BIT_4:	return VK_SAMPLE_COUNT_4_BIT;
		case SAMPLE_COUNT::BIT_8:	return VK_SAMPLE_COUNT_8_BIT;
		case SAMPLE_COUNT::BIT_16:	return VK_SAMPLE_COUNT_16_BIT;
		case SAMPLE_COUNT::BIT_32:	return VK_SAMPLE_COUNT_32_BIT;
		//64 samples not supported for parity with dx12
		default:
		{
			throw std::runtime_error("Default case reached for VulkanPipeline::GetVulkanSampleCount() - sample count = " + std::to_string(std::to_underlying(_count)));
		}
		}
	}



	VkColorComponentFlags VulkanPipeline::GetVulkanColourAspectFlags(COLOUR_ASPECT_FLAGS _mask)
	{
		VkColorComponentFlags result = 0;
		if (EnumUtils::Contains(_mask, COLOUR_ASPECT_FLAGS::R_BIT)) { result |= VK_COLOR_COMPONENT_R_BIT; }
		if (EnumUtils::Contains(_mask, COLOUR_ASPECT_FLAGS::G_BIT)) { result |= VK_COLOR_COMPONENT_G_BIT; }
		if (EnumUtils::Contains(_mask, COLOUR_ASPECT_FLAGS::B_BIT)) { result |= VK_COLOR_COMPONENT_B_BIT; }
		if (EnumUtils::Contains(_mask, COLOUR_ASPECT_FLAGS::A_BIT)) { result |= VK_COLOR_COMPONENT_A_BIT; }
		return result;
	}



	VkBlendFactor VulkanPipeline::GetVulkanBlendFactor(BLEND_FACTOR _factor)
	{
	    switch (_factor)
	    {
	        case BLEND_FACTOR::ZERO:                     return VK_BLEND_FACTOR_ZERO;
	        case BLEND_FACTOR::ONE:                      return VK_BLEND_FACTOR_ONE;
	        case BLEND_FACTOR::SRC_COLOR:                return VK_BLEND_FACTOR_SRC_COLOR;
	        case BLEND_FACTOR::ONE_MINUS_SRC_COLOR:      return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
	        case BLEND_FACTOR::DST_COLOR:                return VK_BLEND_FACTOR_DST_COLOR;
	        case BLEND_FACTOR::ONE_MINUS_DST_COLOR:      return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
	        case BLEND_FACTOR::SRC_ALPHA:                return VK_BLEND_FACTOR_SRC_ALPHA;
	        case BLEND_FACTOR::ONE_MINUS_SRC_ALPHA:      return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	        case BLEND_FACTOR::DST_ALPHA:                return VK_BLEND_FACTOR_DST_ALPHA;
	        case BLEND_FACTOR::ONE_MINUS_DST_ALPHA:      return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
	        case BLEND_FACTOR::CONSTANT_COLOR:           return VK_BLEND_FACTOR_CONSTANT_COLOR;
	        case BLEND_FACTOR::ONE_MINUS_CONSTANT_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
	        case BLEND_FACTOR::CONSTANT_ALPHA:           return VK_BLEND_FACTOR_CONSTANT_ALPHA;
	        case BLEND_FACTOR::ONE_MINUS_CONSTANT_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
	        case BLEND_FACTOR::SRC_ALPHA_SATURATE:       return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
	        case BLEND_FACTOR::SRC1_COLOR:               return VK_BLEND_FACTOR_SRC1_COLOR;
	        case BLEND_FACTOR::ONE_MINUS_SRC1_COLOR:     return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
	        case BLEND_FACTOR::SRC1_ALPHA:               return VK_BLEND_FACTOR_SRC1_ALPHA;
	        case BLEND_FACTOR::ONE_MINUS_SRC1_ALPHA:     return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
	        default:
	        {
	            throw std::runtime_error("Default case reached for VulkanPipeline::GetVulkanBlendFactor() - factor = " + std::to_string(std::to_underlying(_factor)));
	        }
	    }
	}


	VkBlendOp VulkanPipeline::GetVulkanBlendOp(BLEND_OP _op)
	{
		switch (_op)
		{
		case BLEND_OP::ADD:              return VK_BLEND_OP_ADD;
		case BLEND_OP::SUBTRACT:         return VK_BLEND_OP_SUBTRACT;
		case BLEND_OP::REVERSE_SUBTRACT: return VK_BLEND_OP_REVERSE_SUBTRACT;
		case BLEND_OP::MIN:              return VK_BLEND_OP_MIN;
		case BLEND_OP::MAX:              return VK_BLEND_OP_MAX;
		default:
		{
			throw std::runtime_error("Default case reached for VulkanPipeline::GetVulkanBlendOp() - op = " + std::to_string(std::to_underlying(_op)));
		}
		}
	}



	VkLogicOp VulkanPipeline::GetVulkanLogicOp(LOGIC_OP _op)
	{
	    switch (_op)
	    {
	        case LOGIC_OP::CLEAR:          return VK_LOGIC_OP_CLEAR;
	        case LOGIC_OP::AND:            return VK_LOGIC_OP_AND;
	        case LOGIC_OP::AND_REVERSE:    return VK_LOGIC_OP_AND_REVERSE;
	        case LOGIC_OP::COPY:           return VK_LOGIC_OP_COPY;
	        case LOGIC_OP::AND_INVERTED:   return VK_LOGIC_OP_AND_INVERTED;
	        case LOGIC_OP::NO_OP:          return VK_LOGIC_OP_NO_OP;
	        case LOGIC_OP::XOR:            return VK_LOGIC_OP_XOR;
	        case LOGIC_OP::OR:             return VK_LOGIC_OP_OR;
	        case LOGIC_OP::NOR:            return VK_LOGIC_OP_NOR;
	        case LOGIC_OP::EQUIVALENT:     return VK_LOGIC_OP_EQUIVALENT;
	        case LOGIC_OP::INVERT:         return VK_LOGIC_OP_INVERT;
	        case LOGIC_OP::OR_REVERSE:     return VK_LOGIC_OP_OR_REVERSE;
	        case LOGIC_OP::COPY_INVERTED:  return VK_LOGIC_OP_COPY_INVERTED;
	        case LOGIC_OP::OR_INVERTED:    return VK_LOGIC_OP_OR_INVERTED;
	        case LOGIC_OP::NAND:           return VK_LOGIC_OP_NAND;
	        case LOGIC_OP::SET:            return VK_LOGIC_OP_SET;
	        default:
	        {
	            throw std::runtime_error("Default case reached for VulkanPipeline::GetVulkanLogicOp() - op = " + std::to_string(std::to_underlying(_op)));
	        }
	    }
	}
	
}