#include "VulkanUtils.h"

#include <Core/Utils/EnumUtils.h>
#include <RHI/RHIUtils.h>

#include <stdexcept>
#include <string>


namespace NK
{
	
	VkBufferUsageFlags VulkanUtils::GetVulkanBufferUsageFlags(const BUFFER_USAGE_FLAGS _flags)
	{
		VkBufferUsageFlags vkFlags{ 0 };
		
		if (EnumUtils::Contains(_flags, BUFFER_USAGE_FLAGS::TRANSFER_SRC_BIT))		{ vkFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT; }
		if (EnumUtils::Contains(_flags, BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT))		{ vkFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT; }
		if (EnumUtils::Contains(_flags, BUFFER_USAGE_FLAGS::UNIFORM_BUFFER_BIT))	{ vkFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT; }
		if (EnumUtils::Contains(_flags, BUFFER_USAGE_FLAGS::STORAGE_BUFFER_READ_ONLY_BIT))	{ vkFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT; }
		if (EnumUtils::Contains(_flags, BUFFER_USAGE_FLAGS::STORAGE_BUFFER_READ_WRITE_BIT))	{ vkFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT; }
		if (EnumUtils::Contains(_flags, BUFFER_USAGE_FLAGS::VERTEX_BUFFER_BIT))		{ vkFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; }
		if (EnumUtils::Contains(_flags, BUFFER_USAGE_FLAGS::INDEX_BUFFER_BIT))		{ vkFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT; }
		if (EnumUtils::Contains(_flags, BUFFER_USAGE_FLAGS::INDIRECT_BUFFER_BIT))	{ vkFlags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT; }

		return vkFlags;
	}



	BUFFER_USAGE_FLAGS VulkanUtils::GetRHIBufferUsageFlags(const VkBufferUsageFlags _flags)
	{
		BUFFER_USAGE_FLAGS rhiFlags = BUFFER_USAGE_FLAGS::NONE;

		if (_flags & VK_BUFFER_USAGE_TRANSFER_SRC_BIT) { rhiFlags |= BUFFER_USAGE_FLAGS::TRANSFER_SRC_BIT; }
		if (_flags & VK_BUFFER_USAGE_TRANSFER_DST_BIT) { rhiFlags |= BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT; }
		if (_flags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) { rhiFlags |= BUFFER_USAGE_FLAGS::UNIFORM_BUFFER_BIT; }
		if (_flags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) { rhiFlags |= BUFFER_USAGE_FLAGS::STORAGE_BUFFER_READ_WRITE_BIT; }
		if (_flags & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) { rhiFlags |= BUFFER_USAGE_FLAGS::VERTEX_BUFFER_BIT; }
		if (_flags & VK_BUFFER_USAGE_INDEX_BUFFER_BIT) { rhiFlags |= BUFFER_USAGE_FLAGS::INDEX_BUFFER_BIT; }
		if (_flags & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT) { rhiFlags |= BUFFER_USAGE_FLAGS::INDIRECT_BUFFER_BIT; }
		
		return rhiFlags;
	}



	VulkanBarrierInfo VulkanUtils::GetVulkanBarrierInfo(const RESOURCE_STATE _state)
	{
		switch (_state)
		{
		//Common States
		case RESOURCE_STATE::UNDEFINED:			return { VK_IMAGE_LAYOUT_UNDEFINED, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
		case RESOURCE_STATE::PRESENT:			return { VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 0, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };

		//Read-Only States
		case RESOURCE_STATE::VERTEX_BUFFER:		return { VK_IMAGE_LAYOUT_UNDEFINED, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT };
		case RESOURCE_STATE::INDEX_BUFFER:		return { VK_IMAGE_LAYOUT_UNDEFINED, VK_ACCESS_INDEX_READ_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT };
		case RESOURCE_STATE::CONSTANT_BUFFER:	return { VK_IMAGE_LAYOUT_UNDEFINED, VK_ACCESS_UNIFORM_READ_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };
		case RESOURCE_STATE::INDIRECT_BUFFER:	return { VK_IMAGE_LAYOUT_UNDEFINED, VK_ACCESS_INDIRECT_COMMAND_READ_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT };
		case RESOURCE_STATE::SHADER_RESOURCE:	return { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };
		case RESOURCE_STATE::COPY_SOURCE:		return { VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT };
		case RESOURCE_STATE::DEPTH_READ:		return { VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT };

		//Read-Write States
		case RESOURCE_STATE::RENDER_TARGET:		return { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		case RESOURCE_STATE::UNORDERED_ACCESS:	return { VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT };
		case RESOURCE_STATE::COPY_DEST:			return { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT };
		case RESOURCE_STATE::DEPTH_WRITE:		return { VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT };

		default:
		{
			throw std::invalid_argument("GetVulkanBarrierInfo() - provided _state (" + std::to_string(std::to_underlying(_state)) + ") is not yet handled.\n");
		}
		}
	}



	VkIndexType VulkanUtils::GetVulkanIndexType(const DATA_FORMAT _format)
	{
		switch (_format)
		{
		case DATA_FORMAT::R8_UINT:	return VK_INDEX_TYPE_UINT8_EXT;
		case DATA_FORMAT::R16_UINT: return VK_INDEX_TYPE_UINT16;
		case DATA_FORMAT::R32_UINT: return VK_INDEX_TYPE_UINT32;
		default:
		{
			throw std::invalid_argument("Default case reached in GetVulkanIndexType() - _format = " + std::to_string(std::to_underlying(_format)) + "\n");
		}
		}
	}



	DATA_FORMAT VulkanUtils::GetRHIIndexType(const VkIndexType _type)
	{
		switch (_type)
		{
		case VK_INDEX_TYPE_UINT8_EXT:	return DATA_FORMAT::R8_UINT;
		case VK_INDEX_TYPE_UINT16:		return DATA_FORMAT::R16_UINT;
		case VK_INDEX_TYPE_UINT32:		return DATA_FORMAT::R32_UINT;
		default:
		{
			throw std::invalid_argument("Default case reached in GetRHIIndexType() - _type = " + std::to_string(_type) + "\n");
		}
		}
	}



	VkPipelineBindPoint VulkanUtils::GetVulkanPipelineBindPoint(const PIPELINE_BIND_POINT _bindPoint)
	{
		switch (_bindPoint)
		{
		case PIPELINE_BIND_POINT::GRAPHICS: return VK_PIPELINE_BIND_POINT_GRAPHICS;
		case PIPELINE_BIND_POINT::COMPUTE:	return VK_PIPELINE_BIND_POINT_COMPUTE;
		default:
		{
			throw std::invalid_argument("Default case reached in GetVulkanPipelineBindPoint() - _bindPoint = " + std::to_string(std::to_underlying(_bindPoint)) + "\n");
		}
		}
	}



	PIPELINE_BIND_POINT VulkanUtils::GetRHIPipelineBindPoint(const VkPipelineBindPoint _bindPoint)
	{
		switch (_bindPoint)
		{
		case VK_PIPELINE_BIND_POINT_GRAPHICS:	return PIPELINE_BIND_POINT::GRAPHICS;
		case VK_PIPELINE_BIND_POINT_COMPUTE:	return PIPELINE_BIND_POINT::COMPUTE;
		default:
		{
			throw std::invalid_argument("Default case reached in GetRHIPipelineBindPoint() - _bindPoint = " + std::to_string(_bindPoint) + "\n");
		}
		}
	}



	std::size_t VulkanUtils::GetVulkanQueueFamilyIndex(const COMMAND_TYPE _type, const VulkanDevice& _device)
	{
		switch (_type)
		{
		case COMMAND_TYPE::GRAPHICS:	return _device.GetGraphicsQueueFamilyIndex();
		case COMMAND_TYPE::COMPUTE:		return _device.GetComputeQueueFamilyIndex();
		case COMMAND_TYPE::TRANSFER:	return _device.GetTransferQueueFamilyIndex();
		default:
		{
			throw std::invalid_argument("GetQueueFamilyIndex() - switch case returned default. type = " + RHIUtils::GetCommandTypeString(_type));
		}
		}
	}



	VkPrimitiveTopology VulkanUtils::GetVulkanTopology(const INPUT_TOPOLOGY _topology)
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
			throw std::invalid_argument("Default case reached for VulkanUtils::GetVulkanTopology() - topology = " + std::to_string(std::to_underlying(_topology)));
		}
		}
	}



	INPUT_TOPOLOGY VulkanUtils::GetRHITopology(const VkPrimitiveTopology _topology)
	{
		switch (_topology)
		{
		case VK_PRIMITIVE_TOPOLOGY_POINT_LIST: return INPUT_TOPOLOGY::POINT_LIST;
		case VK_PRIMITIVE_TOPOLOGY_LINE_LIST: return INPUT_TOPOLOGY::LINE_LIST;
		case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP: return INPUT_TOPOLOGY::LINE_STRIP;
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: return INPUT_TOPOLOGY::TRIANGLE_LIST;
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: return INPUT_TOPOLOGY::TRIANGLE_STRIP;
		case VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY: return INPUT_TOPOLOGY::LINE_LIST_ADJ;
		case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY: return INPUT_TOPOLOGY::LINE_STRIP_ADJ;
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY: return INPUT_TOPOLOGY::TRIANGLE_LIST_ADJ;
		case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY: return INPUT_TOPOLOGY::TRIANGLE_STRIP_ADJ;
		default:
		{
			throw std::invalid_argument("Default case reached for VulkanUtils::GetRHITopology() - topology = " + std::to_string(_topology));
		}
		}
	}



	VkCullModeFlags VulkanUtils::GetVulkanCullMode(const CULL_MODE _mode)
	{
		switch (_mode)
		{
		case CULL_MODE::NONE:			return VK_CULL_MODE_NONE;
		case CULL_MODE::FRONT:			return VK_CULL_MODE_FRONT_BIT;
		case CULL_MODE::BACK:			return VK_CULL_MODE_BACK_BIT;
		default:
		{
			throw std::invalid_argument("Default case reached for VulkanUtils::GetVulkanCullMode() - cull mode = " + std::to_string(std::to_underlying(_mode)));
		}
		}
	}



	CULL_MODE VulkanUtils::GetRHICullMode(const VkCullModeFlags _mode)
	{
		switch (_mode)
		{
		case VK_CULL_MODE_NONE: return CULL_MODE::NONE;
		case VK_CULL_MODE_FRONT_BIT: return CULL_MODE::FRONT;
		case VK_CULL_MODE_BACK_BIT: return CULL_MODE::BACK;
		default:
		{
			throw std::invalid_argument("Default case reached for VulkanUtils::GetRHICullMode() - cull mode = " + std::to_string(_mode));
		}
		}
	}



	VkFrontFace VulkanUtils::GetVulkanWindingDirection(const WINDING_DIRECTION _direction)
	{
		switch (_direction)
		{
		case WINDING_DIRECTION::CLOCKWISE:			return VK_FRONT_FACE_CLOCKWISE;
		case WINDING_DIRECTION::COUNTER_CLOCKWISE:	return VK_FRONT_FACE_COUNTER_CLOCKWISE;
		default:
		{
			throw std::invalid_argument("Default case reached for VulkanUtils::GetVulkanWindingDirection() - winding direction = " + std::to_string(std::to_underlying(_direction)));
		}
		}
	}



	WINDING_DIRECTION VulkanUtils::GetRHIWindingDirection(const VkFrontFace _direction)
	{
		switch (_direction)
		{
		case VK_FRONT_FACE_CLOCKWISE: return WINDING_DIRECTION::CLOCKWISE;
		case VK_FRONT_FACE_COUNTER_CLOCKWISE: return WINDING_DIRECTION::COUNTER_CLOCKWISE;
		default:
		{
			throw std::invalid_argument("Default case reached for VulkanUtils::GetRHIWindingDirection() - winding direction = " + std::to_string(_direction));
		}
		}
	}



	VkCompareOp VulkanUtils::GetVulkanCompareOp(const COMPARE_OP _op)
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
			throw std::invalid_argument("Default case reached for VulkanUtils::GetVulkanCompareOp() - op = " + std::to_string(std::to_underlying(_op)));
		}
		}
	}



	COMPARE_OP VulkanUtils::GetRHICompareOp(const VkCompareOp _op)
	{
		switch (_op)
		{
		case VK_COMPARE_OP_NEVER: return COMPARE_OP::NEVER;
		case VK_COMPARE_OP_LESS: return COMPARE_OP::LESS;
		case VK_COMPARE_OP_EQUAL: return COMPARE_OP::EQUAL;
		case VK_COMPARE_OP_LESS_OR_EQUAL: return COMPARE_OP::LESS_OR_EQUAL;
		case VK_COMPARE_OP_GREATER: return COMPARE_OP::GREATER;
		case VK_COMPARE_OP_NOT_EQUAL: return COMPARE_OP::NOT_EQUAL;
		case VK_COMPARE_OP_GREATER_OR_EQUAL: return COMPARE_OP::GREATER_OR_EQUAL;
		case VK_COMPARE_OP_ALWAYS: return COMPARE_OP::ALWAYS;
		default:
		{
			throw std::invalid_argument("Default case reached for VulkanUtils::GetRHICompareOp() - op = " + std::to_string(_op));
		}
		}
	}



	VkStencilOp VulkanUtils::GetVulkanStencilOp(const STENCIL_OP _op)
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
			throw std::invalid_argument("Default case reached for VulkanUtils::GetVulkanStencilOp() - op = " + std::to_string(std::to_underlying(_op)));
		}
		}
	}



	STENCIL_OP VulkanUtils::GetRHIStencilOp(const VkStencilOp _op)
	{
		switch (_op)
		{
		case VK_STENCIL_OP_KEEP: return STENCIL_OP::KEEP;
		case VK_STENCIL_OP_ZERO: return STENCIL_OP::ZERO;
		case VK_STENCIL_OP_REPLACE: return STENCIL_OP::REPLACE;
		case VK_STENCIL_OP_INCREMENT_AND_CLAMP: return STENCIL_OP::INCREMENT_AND_CLAMP;
		case VK_STENCIL_OP_DECREMENT_AND_CLAMP: return STENCIL_OP::DECREMENT_AND_CLAMP;
		case VK_STENCIL_OP_INVERT: return STENCIL_OP::INVERT;
		case VK_STENCIL_OP_INCREMENT_AND_WRAP: return STENCIL_OP::INCREMENT_AND_WRAP;
		case VK_STENCIL_OP_DECREMENT_AND_WRAP: return STENCIL_OP::DECREMENT_AND_WRAP;
		default:
		{
			throw std::invalid_argument("Default case reached for VulkanUtils::GetRHIStencilOp() - op = " + std::to_string(_op));
		}
		}
	}



	static std::uint32_t Convert8BitMaskTo32BitMask(std::uint8_t _mask)
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

	VkStencilOpState VulkanUtils::GetVulkanStencilOpState(const StencilOpState _state, const std::uint8_t _readMask, const std::uint8_t _writeMask)
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



	VkSampleCountFlagBits VulkanUtils::GetVulkanSampleCount(const SAMPLE_COUNT _count)
	{
		switch (_count)
		{
		case SAMPLE_COUNT::BIT_1:	return VK_SAMPLE_COUNT_1_BIT;
		case SAMPLE_COUNT::BIT_2:	return VK_SAMPLE_COUNT_2_BIT;
		case SAMPLE_COUNT::BIT_4:	return VK_SAMPLE_COUNT_4_BIT;
		case SAMPLE_COUNT::BIT_8:	return VK_SAMPLE_COUNT_8_BIT;
		case SAMPLE_COUNT::BIT_16:	return VK_SAMPLE_COUNT_16_BIT;
		case SAMPLE_COUNT::BIT_32:	return VK_SAMPLE_COUNT_32_BIT;
		default:
		{
			throw std::invalid_argument("Default case reached for VulkanUtils::GetVulkanSampleCount() - sample count = " + std::to_string(std::to_underlying(_count)));
		}
		}
	}



	SAMPLE_COUNT VulkanUtils::GetRHISampleCount(const VkSampleCountFlagBits _count)
	{
		switch (_count)
		{
		case VK_SAMPLE_COUNT_1_BIT: return SAMPLE_COUNT::BIT_1;
		case VK_SAMPLE_COUNT_2_BIT: return SAMPLE_COUNT::BIT_2;
		case VK_SAMPLE_COUNT_4_BIT: return SAMPLE_COUNT::BIT_4;
		case VK_SAMPLE_COUNT_8_BIT: return SAMPLE_COUNT::BIT_8;
		case VK_SAMPLE_COUNT_16_BIT: return SAMPLE_COUNT::BIT_16;
		case VK_SAMPLE_COUNT_32_BIT: return SAMPLE_COUNT::BIT_32;
		default:
		{
			throw std::invalid_argument("Default case reached for VulkanUtils::GetRHISampleCount() - sample count = " + std::to_string(_count));
		}
		}
	}



	VkColorComponentFlags VulkanUtils::GetVulkanColourAspectFlags(const COLOUR_ASPECT_FLAGS _mask)
	{
		VkColorComponentFlags result = 0;
		if (EnumUtils::Contains(_mask, COLOUR_ASPECT_FLAGS::R_BIT)) { result |= VK_COLOR_COMPONENT_R_BIT; }
		if (EnumUtils::Contains(_mask, COLOUR_ASPECT_FLAGS::G_BIT)) { result |= VK_COLOR_COMPONENT_G_BIT; }
		if (EnumUtils::Contains(_mask, COLOUR_ASPECT_FLAGS::B_BIT)) { result |= VK_COLOR_COMPONENT_B_BIT; }
		if (EnumUtils::Contains(_mask, COLOUR_ASPECT_FLAGS::A_BIT)) { result |= VK_COLOR_COMPONENT_A_BIT; }
		return result;
	}



	COLOUR_ASPECT_FLAGS VulkanUtils::GetRHIColourAspectFlags(const VkColorComponentFlags _mask)
	{
		COLOUR_ASPECT_FLAGS result = COLOUR_ASPECT_FLAGS(0);
		if (_mask & VK_COLOR_COMPONENT_R_BIT) { result |= COLOUR_ASPECT_FLAGS::R_BIT; }
		if (_mask & VK_COLOR_COMPONENT_G_BIT) { result |= COLOUR_ASPECT_FLAGS::G_BIT; }
		if (_mask & VK_COLOR_COMPONENT_B_BIT) { result |= COLOUR_ASPECT_FLAGS::B_BIT; }
		if (_mask & VK_COLOR_COMPONENT_A_BIT) { result |= COLOUR_ASPECT_FLAGS::A_BIT; }
		return result;
	}



	VkBlendFactor VulkanUtils::GetVulkanBlendFactor(const BLEND_FACTOR _factor)
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
	            throw std::invalid_argument("Default case reached for VulkanUtils::GetVulkanBlendFactor() - factor = " + std::to_string(std::to_underlying(_factor)));
	        }
	    }
	}



	BLEND_FACTOR VulkanUtils::GetRHIBlendFactor(const VkBlendFactor _factor)
	{
	    switch (_factor)
	    {
	        case VK_BLEND_FACTOR_ZERO:                     return BLEND_FACTOR::ZERO;
	        case VK_BLEND_FACTOR_ONE:                      return BLEND_FACTOR::ONE;
	        case VK_BLEND_FACTOR_SRC_COLOR:                return BLEND_FACTOR::SRC_COLOR;
	        case VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR:      return BLEND_FACTOR::ONE_MINUS_SRC_COLOR;
	        case VK_BLEND_FACTOR_DST_COLOR:                return BLEND_FACTOR::DST_COLOR;
	        case VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR:      return BLEND_FACTOR::ONE_MINUS_DST_COLOR;
	        case VK_BLEND_FACTOR_SRC_ALPHA:                return BLEND_FACTOR::SRC_ALPHA;
	        case VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:      return BLEND_FACTOR::ONE_MINUS_SRC_ALPHA;
	        case VK_BLEND_FACTOR_DST_ALPHA:                return BLEND_FACTOR::DST_ALPHA;
	        case VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA:      return BLEND_FACTOR::ONE_MINUS_DST_ALPHA;
	        case VK_BLEND_FACTOR_CONSTANT_COLOR:           return BLEND_FACTOR::CONSTANT_COLOR;
	        case VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR: return BLEND_FACTOR::ONE_MINUS_CONSTANT_COLOR;
	        case VK_BLEND_FACTOR_CONSTANT_ALPHA:           return BLEND_FACTOR::CONSTANT_ALPHA;
	        case VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA: return BLEND_FACTOR::ONE_MINUS_CONSTANT_ALPHA;
	        case VK_BLEND_FACTOR_SRC_ALPHA_SATURATE:       return BLEND_FACTOR::SRC_ALPHA_SATURATE;
	        case VK_BLEND_FACTOR_SRC1_COLOR:               return BLEND_FACTOR::SRC1_COLOR;
	        case VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR:     return BLEND_FACTOR::ONE_MINUS_SRC1_COLOR;
	        case VK_BLEND_FACTOR_SRC1_ALPHA:               return BLEND_FACTOR::SRC1_ALPHA;
	        case VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA:     return BLEND_FACTOR::ONE_MINUS_SRC1_ALPHA;
	        default:
	        {
	            throw std::invalid_argument("Default case reached for VulkanUtils::GetRHIBlendFactor() - factor = " + std::to_string(_factor));
	        }
	    }
	}



	VkBlendOp VulkanUtils::GetVulkanBlendOp(const BLEND_OP _op)
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
			throw std::invalid_argument("Default case reached for VulkanUtils::GetVulkanBlendOp() - op = " + std::to_string(std::to_underlying(_op)));
		}
		}
	}



	BLEND_OP VulkanUtils::GetRHIBlendOp(const VkBlendOp _op)
	{
		switch (_op)
		{
		case VK_BLEND_OP_ADD: return BLEND_OP::ADD;
		case VK_BLEND_OP_SUBTRACT: return BLEND_OP::SUBTRACT;
		case VK_BLEND_OP_REVERSE_SUBTRACT: return BLEND_OP::REVERSE_SUBTRACT;
		case VK_BLEND_OP_MIN: return BLEND_OP::MIN;
		case VK_BLEND_OP_MAX: return BLEND_OP::MAX;
		default:
		{
			throw std::invalid_argument("Default case reached for VulkanUtils::GetRHIBlendOp() - op = " + std::to_string(_op));
		}
		}
	}



	VkLogicOp VulkanUtils::GetVulkanLogicOp(const LOGIC_OP _op)
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
	            throw std::invalid_argument("Default case reached for VulkanUtils::GetVulkanLogicOp() - op = " + std::to_string(std::to_underlying(_op)));
	        }
	    }
	}



	LOGIC_OP VulkanUtils::GetRHILogicOp(const VkLogicOp _op)
	{
	    switch (_op)
	    {
	        case VK_LOGIC_OP_CLEAR: return LOGIC_OP::CLEAR;
	        case VK_LOGIC_OP_AND: return LOGIC_OP::AND;
	        case VK_LOGIC_OP_AND_REVERSE: return LOGIC_OP::AND_REVERSE;
	        case VK_LOGIC_OP_COPY: return LOGIC_OP::COPY;
	        case VK_LOGIC_OP_AND_INVERTED: return LOGIC_OP::AND_INVERTED;
	        case VK_LOGIC_OP_NO_OP: return LOGIC_OP::NO_OP;
	        case VK_LOGIC_OP_XOR: return LOGIC_OP::XOR;
	        case VK_LOGIC_OP_OR: return LOGIC_OP::OR;
	        case VK_LOGIC_OP_NOR: return LOGIC_OP::NOR;
	        case VK_LOGIC_OP_EQUIVALENT: return LOGIC_OP::EQUIVALENT;
	        case VK_LOGIC_OP_INVERT: return LOGIC_OP::INVERT;
	        case VK_LOGIC_OP_OR_REVERSE: return LOGIC_OP::OR_REVERSE;
	        case VK_LOGIC_OP_COPY_INVERTED: return LOGIC_OP::COPY_INVERTED;
	        case VK_LOGIC_OP_OR_INVERTED: return LOGIC_OP::OR_INVERTED;
	        case VK_LOGIC_OP_NAND: return LOGIC_OP::NAND;
	        case VK_LOGIC_OP_SET: return LOGIC_OP::SET;
	        default:
	        {
	            throw std::invalid_argument("Default case reached for VulkanUtils::GetRHILogicOp() - op = " + std::to_string(_op));
	        }
	    }
	}



	VkFilter VulkanUtils::GetVulkanFilter(const FILTER_MODE _filterMode)
	{
		switch (_filterMode)
		{
		case FILTER_MODE::NEAREST:	return VK_FILTER_NEAREST;
		case FILTER_MODE::LINEAR:	return VK_FILTER_LINEAR;
		default:
		{
			throw std::invalid_argument("Default case reached for VulkanUtils::GetVulkanFilter() - filter mode = " + std::to_string(std::to_underlying(_filterMode)));
		}
		}
	}



	FILTER_MODE VulkanUtils::GetRHIFilter(const VkFilter _filter)
	{
		switch (_filter)
		{
		case VK_FILTER_NEAREST: return FILTER_MODE::NEAREST;
		case VK_FILTER_LINEAR: return FILTER_MODE::LINEAR;
		default:
		{
			throw std::invalid_argument("Default case reached for VulkanUtils::GetRHIFilter() - filter = " + std::to_string(_filter));
		}
		}
	}



	VkSamplerMipmapMode VulkanUtils::GetVulkanMipmapMode(const FILTER_MODE _filterMode)
	{
		switch (_filterMode)
		{
		case FILTER_MODE::NEAREST:	return VK_SAMPLER_MIPMAP_MODE_NEAREST;
		case FILTER_MODE::LINEAR:	return VK_SAMPLER_MIPMAP_MODE_LINEAR;
		default:
		{
			throw std::invalid_argument("Default case reached for VulkanUtils::GetVulkanMipmapMode() - filter mode = " + std::to_string(std::to_underlying(_filterMode)));
		}
		}
	}



	FILTER_MODE VulkanUtils::GetRHIMipmapMode(const VkSamplerMipmapMode _mipmapMode)
	{
		switch (_mipmapMode)
		{
		case VK_SAMPLER_MIPMAP_MODE_NEAREST: return FILTER_MODE::NEAREST;
		case VK_SAMPLER_MIPMAP_MODE_LINEAR: return FILTER_MODE::LINEAR;
		default:
		{
			throw std::invalid_argument("Default case reached for VulkanUtils::GetRHIMipmapMode() - mipmapMode = " + std::to_string(_mipmapMode));
		}
		}
	}



	VkSamplerAddressMode VulkanUtils::GetVulkanAddressMode(const ADDRESS_MODE _addressMode)
	{
		switch (_addressMode)
		{
		case ADDRESS_MODE::REPEAT:					return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case ADDRESS_MODE::MIRRORED_REPEAT:			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		case ADDRESS_MODE::CLAMP_TO_EDGE:			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case ADDRESS_MODE::CLAMP_TO_BORDER:			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		case ADDRESS_MODE::MIRROR_CLAMP_TO_EDGE:	return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
		default:
		{
			throw std::invalid_argument("Default case reached for VulkanUtils::GetVulkanAddressMode() - address mode = " + std::to_string(std::to_underlying(_addressMode)));
		}
		}
	}



	ADDRESS_MODE VulkanUtils::GetRHIAddressMode(const VkSamplerAddressMode _addressMode)
	{
		switch (_addressMode)
		{
		case VK_SAMPLER_ADDRESS_MODE_REPEAT: return ADDRESS_MODE::REPEAT;
		case VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT: return ADDRESS_MODE::MIRRORED_REPEAT;
		case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE: return ADDRESS_MODE::CLAMP_TO_EDGE;
		case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER: return ADDRESS_MODE::CLAMP_TO_BORDER;
		case VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE: return ADDRESS_MODE::MIRROR_CLAMP_TO_EDGE;
		default:
		{
			throw std::invalid_argument("Default case reached for VulkanUtils::GetRHIAddressMode() - address mode = " + std::to_string(_addressMode));
		}
		}
	}



	VkImageUsageFlags VulkanUtils::GetVulkanImageUsageFlags(const TEXTURE_USAGE_FLAGS _flags)
	{
		VkImageUsageFlags vkFlags{ 0 };
		
		if (EnumUtils::Contains(_flags, TEXTURE_USAGE_FLAGS::TRANSFER_SRC_BIT))			{ vkFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT; }
		if (EnumUtils::Contains(_flags, TEXTURE_USAGE_FLAGS::TRANSFER_DST_BIT))			{ vkFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT; }
		if (EnumUtils::Contains(_flags, TEXTURE_USAGE_FLAGS::READ_ONLY))				{ vkFlags |= VK_IMAGE_USAGE_SAMPLED_BIT; }
		if (EnumUtils::Contains(_flags, TEXTURE_USAGE_FLAGS::READ_WRITE))				{ vkFlags |= VK_IMAGE_USAGE_STORAGE_BIT; }
		if (EnumUtils::Contains(_flags, TEXTURE_USAGE_FLAGS::COLOUR_ATTACHMENT))		{ vkFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; }
		if (EnumUtils::Contains(_flags, TEXTURE_USAGE_FLAGS::DEPTH_STENCIL_ATTACHMENT))	{ vkFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT; }

		return vkFlags;
	}



	TEXTURE_USAGE_FLAGS VulkanUtils::GetRHIImageUsageFlags(const VkImageUsageFlags _flags)
	{
		TEXTURE_USAGE_FLAGS rhiFlags{ TEXTURE_USAGE_FLAGS(0) };

		if (_flags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)				{ rhiFlags |= TEXTURE_USAGE_FLAGS::TRANSFER_SRC_BIT; }
		if (_flags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)				{ rhiFlags |= TEXTURE_USAGE_FLAGS::TRANSFER_DST_BIT; }
		if (_flags & VK_IMAGE_USAGE_SAMPLED_BIT)					{ rhiFlags |= TEXTURE_USAGE_FLAGS::READ_ONLY; }
		if (_flags & VK_IMAGE_USAGE_STORAGE_BIT)					{ rhiFlags |= TEXTURE_USAGE_FLAGS::READ_WRITE; }
		if (_flags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)			{ rhiFlags |= TEXTURE_USAGE_FLAGS::COLOUR_ATTACHMENT; }
		if (_flags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)	{ rhiFlags |= TEXTURE_USAGE_FLAGS::DEPTH_STENCIL_ATTACHMENT; }

		return rhiFlags;
	}



	VkFormat VulkanUtils::GetVulkanFormat(const DATA_FORMAT _format)
	{
		switch (_format)
		{
		case DATA_FORMAT::UNDEFINED:				return VK_FORMAT_UNDEFINED;

		case DATA_FORMAT::R32_TYPELESS:				return VK_FORMAT_R32_SFLOAT;
			
		case DATA_FORMAT::R8_UNORM:					return VK_FORMAT_R8_UNORM;
		case DATA_FORMAT::R8G8_UNORM:				return VK_FORMAT_R8G8_UNORM;
		case DATA_FORMAT::R8G8B8A8_UNORM:			return VK_FORMAT_R8G8B8A8_UNORM;
		case DATA_FORMAT::R8G8B8A8_SRGB:			return VK_FORMAT_R8G8B8A8_SRGB;
		case DATA_FORMAT::B8G8R8A8_UNORM:			return VK_FORMAT_B8G8R8A8_UNORM;
		case DATA_FORMAT::B8G8R8A8_SRGB:			return VK_FORMAT_B8G8R8A8_SRGB;

		case DATA_FORMAT::R16_SFLOAT:				return VK_FORMAT_R16_SFLOAT;
		case DATA_FORMAT::R16G16_SFLOAT:			return VK_FORMAT_R16G16_SFLOAT;
		case DATA_FORMAT::R16G16B16A16_SFLOAT:		return VK_FORMAT_R16G16B16A16_SFLOAT;

		case DATA_FORMAT::R32_SFLOAT:				return VK_FORMAT_R32_SFLOAT;
		case DATA_FORMAT::R32G32_SFLOAT:			return VK_FORMAT_R32G32_SFLOAT;
		case DATA_FORMAT::R32G32B32_SFLOAT:			return VK_FORMAT_R32G32B32_SFLOAT;
		case DATA_FORMAT::R32G32B32A32_SFLOAT:		return VK_FORMAT_R32G32B32A32_SFLOAT;

		case DATA_FORMAT::B10G11R11_UFLOAT_PACK32:	return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
		case DATA_FORMAT::R10G10B10A2_UNORM:		return VK_FORMAT_A2R10G10B10_UNORM_PACK32;

		case DATA_FORMAT::D16_UNORM:				return VK_FORMAT_D16_UNORM;
		case DATA_FORMAT::D32_SFLOAT:				return VK_FORMAT_D32_SFLOAT;
		case DATA_FORMAT::D24_UNORM_S8_UINT:		return VK_FORMAT_D24_UNORM_S8_UINT;

		case DATA_FORMAT::BC1_RGB_UNORM:			return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
		case DATA_FORMAT::BC1_RGB_SRGB:				return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
		case DATA_FORMAT::BC3_RGBA_UNORM:			return VK_FORMAT_BC3_UNORM_BLOCK;
		case DATA_FORMAT::BC3_RGBA_SRGB:			return VK_FORMAT_BC3_SRGB_BLOCK;
		case DATA_FORMAT::BC4_R_UNORM:				return VK_FORMAT_BC4_UNORM_BLOCK;
		case DATA_FORMAT::BC4_R_SNORM:				return VK_FORMAT_BC4_SNORM_BLOCK;
		case DATA_FORMAT::BC5_RG_UNORM:				return VK_FORMAT_BC5_UNORM_BLOCK;
		case DATA_FORMAT::BC5_RG_SNORM:				return VK_FORMAT_BC5_SNORM_BLOCK;
		case DATA_FORMAT::BC7_RGBA_UNORM:			return VK_FORMAT_BC7_UNORM_BLOCK;
		case DATA_FORMAT::BC7_RGBA_SRGB:			return VK_FORMAT_BC7_SRGB_BLOCK;

		case DATA_FORMAT::R8_UINT:					return VK_FORMAT_R8_UINT;
		case DATA_FORMAT::R16_UINT:					return VK_FORMAT_R16_UINT;
		case DATA_FORMAT::R32_UINT:					return VK_FORMAT_R32_UINT;

		default:
		{
			throw std::invalid_argument("GetVulkanFormat() default case reached. Format = " + std::to_string(std::to_underlying(_format)));
		}
		}
	}



	DATA_FORMAT VulkanUtils::GetRHIFormat(const VkFormat _format)
	{
		switch (_format)
		{
		case VK_FORMAT_UNDEFINED:					return DATA_FORMAT::UNDEFINED;

		case VK_FORMAT_R8_UNORM:					return DATA_FORMAT::R8_UNORM;
		case VK_FORMAT_R8G8_UNORM:					return DATA_FORMAT::R8G8_UNORM;
		case VK_FORMAT_R8G8B8A8_UNORM:				return DATA_FORMAT::R8G8B8A8_UNORM;
		case VK_FORMAT_R8G8B8A8_SRGB:				return DATA_FORMAT::R8G8B8A8_SRGB;
		case VK_FORMAT_B8G8R8A8_UNORM:				return DATA_FORMAT::B8G8R8A8_UNORM;
		case VK_FORMAT_B8G8R8A8_SRGB:				return DATA_FORMAT::B8G8R8A8_SRGB;

		case VK_FORMAT_R16_SFLOAT:					return DATA_FORMAT::R16_SFLOAT;
		case VK_FORMAT_R16G16_SFLOAT:				return DATA_FORMAT::R16G16_SFLOAT;
		case VK_FORMAT_R16G16B16A16_SFLOAT:			return DATA_FORMAT::R16G16B16A16_SFLOAT;

		case VK_FORMAT_R32_SFLOAT:					return DATA_FORMAT::R32_SFLOAT;
		case VK_FORMAT_R32G32_SFLOAT:				return DATA_FORMAT::R32G32_SFLOAT;
		case VK_FORMAT_R32G32B32A32_SFLOAT:			return DATA_FORMAT::R32G32B32A32_SFLOAT;

		case VK_FORMAT_B10G11R11_UFLOAT_PACK32:		return DATA_FORMAT::B10G11R11_UFLOAT_PACK32;
		case VK_FORMAT_A2R10G10B10_UNORM_PACK32:	return DATA_FORMAT::R10G10B10A2_UNORM;

		case VK_FORMAT_D16_UNORM:					return DATA_FORMAT::D16_UNORM;
		case VK_FORMAT_D32_SFLOAT:					return DATA_FORMAT::D32_SFLOAT;
		case VK_FORMAT_D24_UNORM_S8_UINT:			return DATA_FORMAT::D24_UNORM_S8_UINT;

		case VK_FORMAT_BC1_RGB_UNORM_BLOCK:			return DATA_FORMAT::BC1_RGB_UNORM;
		case VK_FORMAT_BC1_RGB_SRGB_BLOCK:			return DATA_FORMAT::BC1_RGB_SRGB;
		case VK_FORMAT_BC3_UNORM_BLOCK:				return DATA_FORMAT::BC3_RGBA_UNORM;
		case VK_FORMAT_BC3_SRGB_BLOCK:				return DATA_FORMAT::BC3_RGBA_SRGB;
		case VK_FORMAT_BC4_UNORM_BLOCK:				return DATA_FORMAT::BC4_R_UNORM;
		case VK_FORMAT_BC4_SNORM_BLOCK:				return DATA_FORMAT::BC4_R_SNORM;
		case VK_FORMAT_BC5_UNORM_BLOCK:				return DATA_FORMAT::BC5_RG_UNORM;
		case VK_FORMAT_BC5_SNORM_BLOCK:				return DATA_FORMAT::BC5_RG_SNORM;
		case VK_FORMAT_BC7_UNORM_BLOCK:				return DATA_FORMAT::BC7_RGBA_UNORM;
		case VK_FORMAT_BC7_SRGB_BLOCK:				return DATA_FORMAT::BC7_RGBA_SRGB;

		case VK_FORMAT_R32_UINT:					return DATA_FORMAT::R32_UINT;

		default:
		{
			throw std::invalid_argument("GetRHIFormat() default case reached. Format = " + std::to_string(_format));
		}
		}
	}



	VkImageAspectFlags VulkanUtils::GetVulkanAspectFromRHIFormat(const DATA_FORMAT _format)
	{
		switch (_format)
		{
		case DATA_FORMAT::UNDEFINED:
			return VK_IMAGE_ASPECT_NONE;

		case DATA_FORMAT::R8_UNORM:
		case DATA_FORMAT::R8G8_UNORM:
		case DATA_FORMAT::R8G8B8A8_UNORM:
		case DATA_FORMAT::R8G8B8A8_SRGB:
		case DATA_FORMAT::B8G8R8A8_UNORM:
		case DATA_FORMAT::B8G8R8A8_SRGB:
		case DATA_FORMAT::R16_SFLOAT:
		case DATA_FORMAT::R16G16_SFLOAT:
		case DATA_FORMAT::R16G16B16A16_SFLOAT:
		case DATA_FORMAT::R32_SFLOAT:
		case DATA_FORMAT::R32G32_SFLOAT:
		case DATA_FORMAT::R32G32B32_SFLOAT:
		case DATA_FORMAT::R32G32B32A32_SFLOAT:
		case DATA_FORMAT::B10G11R11_UFLOAT_PACK32:
		case DATA_FORMAT::R10G10B10A2_UNORM:
		case DATA_FORMAT::BC1_RGB_UNORM:
		case DATA_FORMAT::BC1_RGB_SRGB:
		case DATA_FORMAT::BC3_RGBA_UNORM:
		case DATA_FORMAT::BC3_RGBA_SRGB:
		case DATA_FORMAT::BC4_R_UNORM:
		case DATA_FORMAT::BC4_R_SNORM:
		case DATA_FORMAT::BC5_RG_UNORM:
		case DATA_FORMAT::BC5_RG_SNORM:
		case DATA_FORMAT::BC7_RGBA_UNORM:
		case DATA_FORMAT::BC7_RGBA_SRGB:
		case DATA_FORMAT::R8_UINT:
		case DATA_FORMAT::R16_UINT:
		case DATA_FORMAT::R32_UINT:
			return VK_IMAGE_ASPECT_COLOR_BIT;

		case DATA_FORMAT::D16_UNORM:
		case DATA_FORMAT::D32_SFLOAT:
			return VK_IMAGE_ASPECT_DEPTH_BIT;

		case DATA_FORMAT::D24_UNORM_S8_UINT:
			return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

		default:
		{
			throw std::invalid_argument("GetVulkanAspectFromRHIFormat() default case reached. Format = " + std::to_string(std::to_underlying(_format)));
		}
		}
	}



	VkImageViewType VulkanUtils::GetVulkanImageViewType(const TEXTURE_VIEW_DIMENSION _dimension)
	{
		switch (_dimension)
		{
		case TEXTURE_VIEW_DIMENSION::DIM_1:				return VK_IMAGE_VIEW_TYPE_1D;
		case TEXTURE_VIEW_DIMENSION::DIM_2:				return VK_IMAGE_VIEW_TYPE_2D;
		case TEXTURE_VIEW_DIMENSION::DIM_3:				return VK_IMAGE_VIEW_TYPE_3D;
		case TEXTURE_VIEW_DIMENSION::DIM_CUBE:			return VK_IMAGE_VIEW_TYPE_CUBE;
		case TEXTURE_VIEW_DIMENSION::DIM_1D_ARRAY:		return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
		case TEXTURE_VIEW_DIMENSION::DIM_2D_ARRAY:		return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		case TEXTURE_VIEW_DIMENSION::DIM_CUBE_ARRAY:	return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;

		default:
		{
			throw std::invalid_argument("GetVulkanImageViewType() default case reached. Dimension = " + std::to_string(std::to_underlying(_dimension)));
		}
		}
	}



	TEXTURE_VIEW_DIMENSION VulkanUtils::GetRHIImageViewDimension(const VkImageViewType _type)
	{
		switch (_type)
		{
		case VK_IMAGE_VIEW_TYPE_1D:         return TEXTURE_VIEW_DIMENSION::DIM_1;
		case VK_IMAGE_VIEW_TYPE_2D:         return TEXTURE_VIEW_DIMENSION::DIM_2;
		case VK_IMAGE_VIEW_TYPE_3D:         return TEXTURE_VIEW_DIMENSION::DIM_3;
		case VK_IMAGE_VIEW_TYPE_CUBE:       return TEXTURE_VIEW_DIMENSION::DIM_CUBE;
		case VK_IMAGE_VIEW_TYPE_1D_ARRAY:   return TEXTURE_VIEW_DIMENSION::DIM_1D_ARRAY;
		case VK_IMAGE_VIEW_TYPE_2D_ARRAY:   return TEXTURE_VIEW_DIMENSION::DIM_2D_ARRAY;
		case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY: return TEXTURE_VIEW_DIMENSION::DIM_CUBE_ARRAY;

		default:
		{
			throw std::invalid_argument("GetRHIImageViewDimension() default case reached. Type = " + std::to_string(std::to_underlying(_type)));
		}
		}
	}

	

	VkImageAspectFlags VulkanUtils::GetVulkanImageAspectFlags(const TEXTURE_ASPECT _aspect)
	{
		switch (_aspect)
		{
		case TEXTURE_ASPECT::COLOUR:        return VK_IMAGE_ASPECT_COLOR_BIT;
		case TEXTURE_ASPECT::DEPTH:         return VK_IMAGE_ASPECT_DEPTH_BIT;
		case TEXTURE_ASPECT::STENCIL:       return VK_IMAGE_ASPECT_STENCIL_BIT;
		case TEXTURE_ASPECT::DEPTH_STENCIL: return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		default:
		{
			throw std::invalid_argument("GetVulkanImageAspectFlags() default case reached. Aspect = " + std::to_string(std::to_underlying(_aspect)));
		}
		}
	}


	
	TEXTURE_ASPECT VulkanUtils::GetRHIImageAspect(const VkImageAspectFlags _flags)
	{
		if (_flags == (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT))
		{
			return TEXTURE_ASPECT::DEPTH_STENCIL;
		}
		if (_flags == VK_IMAGE_ASPECT_DEPTH_BIT)
		{
			return TEXTURE_ASPECT::DEPTH;
		}
		if (_flags == VK_IMAGE_ASPECT_STENCIL_BIT)
		{
			return TEXTURE_ASPECT::STENCIL;
		}
		if (_flags == VK_IMAGE_ASPECT_COLOR_BIT)
		{
			return TEXTURE_ASPECT::COLOUR;
		}

		throw std::invalid_argument("GetRHIImageAspect() invalid or unhandled flags provided. Flags = " + std::to_string(_flags));
	}
	
}