#pragma once
#include <Core/Memory/IAllocator.h>
#include <Types/DataFormat.h>
#include <Types/ShaderAttribute.h>
#include "IDevice.h"
#include "IShader.h"

namespace NK
{
	enum class PIPELINE_TYPE
	{
		GRAPHICS,
		COMPUTE,
	};


	struct VertexAttributeDesc
	{
		std::size_t binding;
		SHADER_ATTRIBUTE attribute;
		DATA_FORMAT format;
		std::uint32_t offset;
	};

	enum class VERTEX_INPUT_RATE
	{
		VERTEX,
		INSTANCE,
	};

	struct VertexBufferBindingDesc
	{
		std::size_t binding;
		std::uint32_t stride;
		VERTEX_INPUT_RATE inputRate;
	};

	struct VertexInputDesc
	{
		std::vector<VertexAttributeDesc> attributeDescriptions;
		std::vector<VertexBufferBindingDesc> bufferBindingDescriptions;
	};


	enum class INPUT_TOPOLOGY
	{
		POINT_LIST,
		LINE_LIST,
		LINE_STRIP,
		TRIANGLE_LIST,
		TRIANGLE_STRIP,

		LINE_LIST_ADJ,
		LINE_STRIP_ADJ,
		TRIANGLE_LIST_ADJ,
		TRIANGLE_STRIP_ADJ,

		//todo: add patch topology types
	};

	struct InputAssemblyDesc
	{
		INPUT_TOPOLOGY topology;
	};


	enum class CULL_MODE
	{
		NONE,
		FRONT,
		BACK,
		FRONT_AND_BACK,
	};

	enum class WINDING_DIRECTION
	{
		CLOCKWISE,
		COUNTER_CLOCKWISE,
	};
	
	struct RasteriserDesc
	{
		CULL_MODE cullMode;
		WINDING_DIRECTION frontFace;
		bool depthBiasEnable;
		float depthBiasConstantFactor;
		float depthBiasClamp;
		float depthBiasSlopeFactor;
	};


	enum class COMPARE_OP
	{
		NEVER,
		LESS,
		EQUAL,
		LESS_OR_EQUAL,
		GREATER,
		NOT_EQUAL,
		GREATER_OR_EQUAL,
		ALWAYS,
	};

	enum class STENCIL_OP
	{
		KEEP,
		ZERO,
		REPLACE,
		INCREMENT_AND_CLAMP,
		DECREMENT_AND_CLAMP,
		INVERT,
		INCREMENT_AND_WRAP,
		DECREMENT_AND_WRAP,
	};

	struct StencilOpState
	{
		STENCIL_OP failOp;
		STENCIL_OP passOp;
		STENCIL_OP depthFailOp;
		COMPARE_OP compareOp;
	};
	
	struct DepthStencilDesc
	{
		bool depthTestEnable;
		bool depthWriteEnable;
		COMPARE_OP depthCompareOp;
		bool stencilTestEnable;
		std::uint8_t stencilReadMask;
		std::uint8_t stencilWriteMask;
		StencilOpState stencilFrontFace;
		StencilOpState stencilBackFace;
	};


	enum class SAMPLE_COUNT
	{
		BIT_1	= 1 << 0,
		BIT_2	= 1 << 1,
		BIT_4	= 1 << 2,
		BIT_8	= 1 << 3,
		BIT_16	= 1 << 4,
		BIT_32	= 1 << 5,
		BIT_64	= 1 << 6,
	};

	typedef std::uint64_t SampleMask;
	
	struct MultisamplingDesc
	{
		SAMPLE_COUNT sampleCount;
		bool supersamplingEnable;
		float supersamplingMinSampleShading;
		SampleMask sampleMask;
		bool alphaToCoverageEnable;
		bool alphaToOneEnable;
	};


	enum class COLOUR_ASPECT_FLAGS
	{
		R_BIT = 1 << 0,
		G_BIT = 1 << 1,
		B_BIT = 1 << 2,
		A_BIT = 1 << 3,
	};
}

//Enable bitmask operators for the COLOUR_ASPECT_FLAGS type
template<>
struct enable_bitmask_operators<NK::COLOUR_ASPECT_FLAGS> : std::true_type {};

namespace NK
{
	enum class BLEND_FACTOR
	{
		ZERO,
		ONE,
		SRC_COLOR,
		ONE_MINUS_SRC_COLOR,
		DST_COLOR,
		ONE_MINUS_DST_COLOR,
		SRC_ALPHA,
		ONE_MINUS_SRC_ALPHA,
		DST_ALPHA,
		ONE_MINUS_DST_ALPHA,
		CONSTANT_COLOR,
		ONE_MINUS_CONSTANT_COLOR,
		CONSTANT_ALPHA,
		ONE_MINUS_CONSTANT_ALPHA,
		SRC_ALPHA_SATURATE,
		SRC1_COLOR,
		ONE_MINUS_SRC1_COLOR,
		SRC1_ALPHA,
		ONE_MINUS_SRC1_ALPHA,
	};

	enum class BLEND_OP
	{
		ADD,
		SUBTRACT,
		REVERSE_SUBTRACT,
		MIN,
		MAX,
	};
	
	struct ColourBlendAttachmentDesc
	{
		COLOUR_ASPECT_FLAGS colourWriteMask;
		bool blendEnable;

		BLEND_FACTOR srcColourBlendFactor;
		BLEND_FACTOR dstColourBlendFactor;
		BLEND_OP colourBlendOp;

		BLEND_FACTOR srcAlphaBlendFactor;
		BLEND_FACTOR dstAlphaBlendFactor;
		BLEND_OP alphaBlendOp;
	};

	enum class LOGIC_OP
	{
		CLEAR,
		AND,
		AND_REVERSE,
		COPY,
		AND_INVERTED,
		NO_OP,
		XOR,
		OR,
		NOR,
		EQUIVALENT,
		INVERT,
		OR_REVERSE,
		COPY_INVERTED,
		OR_INVERTED,
		NAND,
		SET,
	};
	
	struct ColourBlendDesc
	{
		bool logicOpEnable;
		LOGIC_OP logicOp;
		std::vector<ColourBlendAttachmentDesc> attachments;
	};


	struct PipelineDesc
	{
		PIPELINE_TYPE type;

		IShader* computeShader;
		IShader* vertexShader;
		IShader* fragmentShader;
		
		VertexInputDesc vertexInputDesc;
		InputAssemblyDesc inputAssemblyDesc;
		RasteriserDesc rasteriserDesc;
		DepthStencilDesc depthStencilDesc;
		MultisamplingDesc multisamplingDesc;
		ColourBlendDesc colourBlendDesc;

		std::vector<DATA_FORMAT> colourAttachmentFormats;
		DATA_FORMAT depthAttachmentFormat;
		DATA_FORMAT stencilAttachmentFormat;
	};


	class IPipeline
	{
	public:
		virtual ~IPipeline() = default;


	protected:
		IPipeline(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const PipelineDesc& _desc)
		: m_logger(_logger), m_allocator(_allocator), m_device(_device),
		  m_type(_desc.type),
		  m_vertexInputDesc(_desc.vertexInputDesc), m_inputAssemblyDesc(_desc.inputAssemblyDesc), m_rasteriserDesc(_desc.rasteriserDesc), m_depthStencilDesc(_desc.depthStencilDesc), m_multisamplingDesc(_desc.multisamplingDesc), m_colourBlendDesc(_desc.colourBlendDesc),
		  m_colourAttachmentFormats(_desc.colourAttachmentFormats), m_depthAttachmentFormat(_desc.depthAttachmentFormat), m_stencilAttachmentFormat(_desc.stencilAttachmentFormat) {}

		
		//Dependency injections
		ILogger& m_logger;
		IAllocator& m_allocator;
		IDevice& m_device;

		PIPELINE_TYPE m_type;
		
		VertexInputDesc m_vertexInputDesc;
		InputAssemblyDesc m_inputAssemblyDesc;
		RasteriserDesc m_rasteriserDesc;
		DepthStencilDesc m_depthStencilDesc;
		MultisamplingDesc m_multisamplingDesc;
		ColourBlendDesc m_colourBlendDesc;
		
		std::vector<DATA_FORMAT> m_colourAttachmentFormats;
		DATA_FORMAT m_depthAttachmentFormat;
		DATA_FORMAT m_stencilAttachmentFormat;
	};
}