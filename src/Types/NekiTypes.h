#pragma once

#include "ShaderAttributeLocations.h"

#include <Core/Utils/enum_enable_bitmask_operators.h>

#include <cstdint>
#include <typeindex>
#include <variant>
#include <vector>
#include <glm/glm.hpp>


namespace NK
{
	
	//Bitfield of states a buffer is allowed to occupy
	//For compatibility, this must be specified at creation and cannot be changed
	enum class BUFFER_USAGE_FLAGS : std::uint32_t
	{
		NONE				= 0,
		TRANSFER_SRC_BIT	= 1 << 0,
		TRANSFER_DST_BIT	= 1 << 1,
		UNIFORM_BUFFER_BIT	= 1 << 2,
		STORAGE_BUFFER_BIT	= 1 << 3,
		VERTEX_BUFFER_BIT	= 1 << 4,
		INDEX_BUFFER_BIT	= 1 << 5,
		INDIRECT_BUFFER_BIT	= 1 << 6,
	};
	ENABLE_BITMASK_OPERATORS(BUFFER_USAGE_FLAGS)
	
	enum class MEMORY_TYPE
	{
		HOST,
		DEVICE,
	};

	enum class BUFFER_VIEW_TYPE
	{
		UNIFORM,
		STORAGE_READ_ONLY,
		STORAGE_READ_WRITE,
	};
	
	enum class COMMAND_BUFFER_LEVEL
	{
		PRIMARY,
		SECONDARY,
	};

	enum class PIPELINE_BIND_POINT
	{
		GRAPHICS,
		COMPUTE,
	};
	
	enum class COMMAND_TYPE : std::uint32_t
	{
		GRAPHICS,
		COMPUTE,
		TRANSFER,
	};

	enum class COMMAND_POOL_RESET_FLAGS : std::uint32_t
	{
		NONE,
		RELEASE_RESOURCES,
	};
	
	enum class DATA_FORMAT : std::uint32_t
	{
		UNDEFINED = 0,

		//8-bit Colour Formats
		R8_UNORM,
		R8G8_UNORM,
		R8G8B8A8_UNORM,
		R8G8B8A8_SRGB,
		B8G8R8A8_UNORM,
		B8G8R8A8_SRGB,

		//16-bit Colour Formats
		R16_SFLOAT,
		R16G16_SFLOAT,
		R16G16B16A16_SFLOAT,

		//32-bit Colour Formats
		R32_SFLOAT,
		R32G32_SFLOAT,
		R32G32B32_SFLOAT,
		R32G32B32A32_SFLOAT,

		//Packed Formats
		B10G11R11_UFLOAT_PACK32,
		R10G10B10A2_UNORM,

		//Depth/Stencil Formats
		D16_UNORM,
		D32_SFLOAT,
		D24_UNORM_S8_UINT,

		//Block Compression / DXT Formats
		BC1_RGB_UNORM,
		BC1_RGB_SRGB,
		BC3_RGBA_UNORM,
		BC3_RGBA_SRGB,
		BC4_R_UNORM,
		BC4_R_SNORM,
		BC5_RG_UNORM,
		BC5_RG_SNORM,
		BC7_RGBA_UNORM,
		BC7_RGBA_SRGB,

		//Integer Formats
		R8_UINT,
		R16_UINT,
		R32_UINT,
	};

	struct TextureCopyMemoryLayout
	{
		std::uint32_t totalBytes;
		std::uint32_t rowPitch;
	};

	enum class PIPELINE_TYPE
	{
		GRAPHICS,
		COMPUTE,
	};

	enum class SHADER_ATTRIBUTE : std::uint32_t
	{
		POSITION = SHADER_ATTRIBUTE_LOCATION_POSITION,
		NORMAL = SHADER_ATTRIBUTE_LOCATION_NORMAL,
		TEXCOORD_0 = SHADER_ATTRIBUTE_LOCATION_TEXCOORD_0,
		TANGENT = SHADER_ATTRIBUTE_LOCATION_TANGENT,
		BITANGENT = SHADER_ATTRIBUTE_LOCATION_BITANGENT,
		COLOUR_0 = SHADER_ATTRIBUTE_LOCATION_COLOUR_0,
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
	};

	typedef std::uint32_t SampleMask;
	
	struct MultisamplingDesc
	{
		SAMPLE_COUNT sampleCount;
		SampleMask sampleMask;
		bool alphaToCoverageEnable;
	};
    
	enum class COLOUR_ASPECT_FLAGS
	{
		R_BIT = 1 << 0,
		G_BIT = 1 << 1,
		B_BIT = 1 << 2,
		A_BIT = 1 << 3,
	};
	ENABLE_BITMASK_OPERATORS(COLOUR_ASPECT_FLAGS)
	
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

	enum class FILTER_MODE
	{
		NEAREST,
		LINEAR,
	};

	enum class ADDRESS_MODE
	{
		REPEAT,
		MIRRORED_REPEAT,
		CLAMP_TO_EDGE,
		CLAMP_TO_BORDER,
		MIRROR_CLAMP_TO_EDGE,
	};

	enum class SHADER_TYPE
	{
		VERTEX,
		TESS_CTRL,
		TESS_EVAL,
		FRAGMENT,

		COMPUTE,
	};

	enum class TEXTURE_USAGE_FLAGS : std::uint32_t
	{
		NONE                     = 1 << 0,
		TRANSFER_SRC_BIT         = 1 << 1,
		TRANSFER_DST_BIT         = 1 << 2,
		READ_ONLY                = 1 << 3,
		READ_WRITE               = 1 << 4,
		COLOUR_ATTACHMENT        = 1 << 5,
		DEPTH_STENCIL_ATTACHMENT = 1 << 6,
	};
	ENABLE_BITMASK_OPERATORS(TEXTURE_USAGE_FLAGS)
	
	enum class TEXTURE_DIMENSION
	{
		DIM_1,
		DIM_2,
		DIM_3,
	};

	enum class TEXTURE_VIEW_TYPE
	{
		RENDER_TARGET,
		DEPTH,
		DEPTH_STENCIL,
		
		SHADER_READ_ONLY,
		SHADER_READ_WRITE,
	};

	typedef std::uint32_t ResourceIndex;
	typedef std::uint32_t SamplerIndex;
	
	//All the states that a resource can occupy
	enum class RESOURCE_STATE
	{
		UNDEFINED,
		
		//Read-only
		VERTEX_BUFFER,
		INDEX_BUFFER,
		CONSTANT_BUFFER,
		INDIRECT_BUFFER,
		SHADER_RESOURCE, //Equivalent to readonly vulkan storage buffer
		COPY_SOURCE,
		DEPTH_READ,

		//Read/write
		RENDER_TARGET,
		UNORDERED_ACCESS, //Equivalent to readwrite vulkan storage buffer
		COPY_DEST,
		DEPTH_WRITE,

		PRESENT,
	};

	//Defines the severity of a log message
	enum class LOGGER_CHANNEL : std::uint32_t
	{
		NONE	= 0,
		HEADING = 1 << 0,
		INFO	= 1 << 1,
		WARNING = 1 << 2,
		ERROR   = 1 << 3,
		SUCCESS = 1 << 4,
	};
	ENABLE_BITMASK_OPERATORS(LOGGER_CHANNEL)

	enum class LOGGER_LAYER : std::uint32_t
	{
		UNKNOWN,

		VULKAN_GENERAL,
		VULKAN_VALIDATION,
		VULKAN_PERFORMANCE,

		D3D12_APP_DEFINED,
		D3D12_MISC,
		D3D12_INIT,
		D3D12_CLEANUP,
		D3D12_COMPILATION,
		D3D12_STATE_CREATE,
		D3D12_STATE_SET,
		D3D12_STATE_GET,
		D3D12_RES_MANIP,
		D3D12_EXECUTION,
		D3D12_SHADER,

		GLFW,

		ENGINE,
		CONTEXT,
		TRACKING_ALLOCATOR,

		RENDER_LAYER,
		CLIENT_NETWORK_LAYER,
		SERVER_NETWORK_LAYER,
		INPUT_LAYER,
		PLAYER_CAMERA_LAYER,
		DISK_MODEL_LOADER_LAYER,
		
		DEVICE,
		COMMAND_POOL,
		COMMAND_BUFFER,
		BUFFER,
		BUFFER_VIEW,
		TEXTURE,
		TEXTURE_VIEW,
		SAMPLER,
		SURFACE,
		SWAPCHAIN,
		SHADER,
		ROOT_SIGNATURE,
		PIPELINE,
		QUEUE,
		FENCE,
		SEMAPHORE,

		GPU_UPLOADER,
		WINDOW,
		
		APPLICATION,
	};
	
	enum class LOGGER_TYPE
	{
		CONSOLE,
	};
	
	enum class TRACKING_ALLOCATOR_VERBOSITY_FLAGS : std::uint32_t
	{
		NONE =		0,
		ENGINE =	1 << 0,
		VULKAN =	1 << 1,
		VMA =		1 << 2,
		ALL =		UINT32_MAX
	};
	ENABLE_BITMASK_OPERATORS(TRACKING_ALLOCATOR_VERBOSITY_FLAGS)
	
	struct TrackingAllocatorConfig
	{
		TRACKING_ALLOCATOR_VERBOSITY_FLAGS verbosityFlags;
	};

	enum class ALLOCATOR_TYPE
	{
		TRACKING,
	};
	
	struct AllocatorConfig
	{
		ALLOCATOR_TYPE type;
		TrackingAllocatorConfig trackingAllocator;
	};

	enum class ALLOCATION_SOURCE
	{
		UNKNOWN,
		ENGINE,
		VULKAN,
		VMA,
	};

	enum class PROJECTION_METHOD
	{
		ORTHOGRAPHIC,
		PERSPECTIVE,
	};

	struct CameraShaderData
	{
		glm::mat4 viewMat;
		glm::mat4 projMat;
		glm::vec4 pos;
	};

	enum class TEXTURE_VIEW_DIMENSION
	{
		DIM_1,
		DIM_2,
		DIM_3,
		DIM_CUBE,
		DIM_1D_ARRAY,
		DIM_2D_ARRAY,
		DIM_CUBE_ARRAY,
	};

	enum class TEXTURE_ASPECT
	{
		COLOUR,
		DEPTH,
		STENCIL,
		DEPTH_STENCIL,
	};

	enum class GRAPHICS_BACKEND
	{
		NONE,
		VULKAN,
		D3D12,
	};

	typedef std::uint32_t ClientIndex;

	enum class SERVER_TYPE
	{
		LAN,
		WAN, //Requires router port-forwarding
	};

	enum class NETWORK_SYSTEM_TYPE
	{
		NONE,
		SERVER,
		CLIENT,
	};

	enum class NETWORK_LAYER_ERROR_CODE
	{
		SUCCESS,

		SERVER__TCP_LISTENER_PORT_CLAIM_TIME_OUT,
		SERVER__UDP_SOCKET_PORT_CLAIM_TIME_OUT,
		SERVER__FAILED_TO_SEND_CLIENT_INDEX_PACKET,
		SERVER__UDP_FAILED_TO_RECEIVE_PACKET,
		SERVER__HOST_CALLED_ON_HOSTING_SERVER,
		
		CLIENT__INVALID_SERVER_IP_ADDRESS,
		CLIENT__SERVER_CONNECTION_TIMED_OUT,
		CLIENT__CLIENT_INDEX_PACKET_RETRIEVAL_TIMED_OUT,
		CLIENT__FAILED_TO_SEND_UDP_PORT_PACKET,
		CLIENT__FAILED_TO_DISCONNECT_FROM_SERVER,
	};

	enum class PACKET_CODE : std::uint32_t
	{
		//TCP
		DISCONNECT,

		//UDP
		UDP_PORT,
	};

	enum class CLIENT_STATE
	{
		CONNECTING,
		CONNECTED,
		DISCONNECTING,
		DISCONNECTED, //Default state until Connect() is called
	};

	enum class SERVER_STATE
	{
		HOSTING,
		NOT_HOSTING,
	};

	enum class KEYBOARD
	{
		Q,W,E,R,T,Y,U,I,O,P,
		A,S,D,F,G,H,J,K,L,
		Z,X,C,V,B,N,M,
		NUM_1, NUM_2, NUM_3, NUM_4, NUM_5, NUM_6, NUM_7, NUM_8, NUM_9, NUM_0,
		ESCAPE,
	};

	enum class MOUSE_BUTTON
	{
		LEFT,
		RIGHT,
	};
	
	enum class MOUSE
	{
		POSITION,
		POSITION_DIFFERENCE, //Difference in position between last and current frame - call NK::InputManager::UpdateMouseDiff() every frame if using this input
	};
	
	typedef std::variant<KEYBOARD, MOUSE_BUTTON, MOUSE> INPUT_VARIANT;

	enum class INPUT_VARIANT_ENUM_TYPE
	{
		NONE, //Used as default value
		KEYBOARD,
		MOUSE_BUTTON,
		MOUSE,
	};

	struct ButtonState
	{
		//todo: figure out the best way to add a pressed state for first frame
		
		bool held; //Button is currently held down
		bool released; //Button was released this frame
	};

	struct Axis1DState
	{
		float value;
	};

	struct Axis2DState
	{
		glm::vec2 values;
	};

	enum class INPUT_BINDING_TYPE
	{
		BUTTON,
		AXIS_1D,
		AXIS_2D,
		UNBOUND,
	};

	typedef std::variant<ButtonState, Axis1DState, Axis2DState> INPUT_STATE_VARIANT;
	
}

//std::pair doesn't have a default hashing function for whatever reason
//Solution adapted from https://stackoverflow.com/a/17017281
typedef std::pair<std::type_index, std::uint32_t> ActionTypeMapKey;
template<>
struct std::hash<ActionTypeMapKey>
{
	[[nodiscard]] inline std::size_t operator()(const ActionTypeMapKey& _key) const noexcept
	{
		//Compute individual hash values for first and second and combine them using XOR and bit shifting
		return (std::hash<std::type_index>{}(_key.first) ^ (std::hash<std::uint32_t>()(_key.second)) << 1);
	}
};

namespace NK
{

	enum class PLAYER_CAMERA_ACTIONS
	{
		MOVE,
		YAW_PITCH,
	};
	
}