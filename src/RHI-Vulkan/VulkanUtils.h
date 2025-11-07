#pragma once

#include "VulkanDevice.h"

#include <string>


namespace NK
{
	
	struct VulkanBarrierInfo
	{
		VkImageLayout layout;
		VkAccessFlags accessMask;
		VkPipelineStageFlags stageMask;
	};
	
	
	class VulkanUtils
	{
	public:
		[[nodiscard]] static VkBufferUsageFlags GetVulkanBufferUsageFlags(const BUFFER_USAGE_FLAGS _flags);
		[[nodiscard]] static BUFFER_USAGE_FLAGS GetRHIBufferUsageFlags(const VkBufferUsageFlags _flags);

		[[nodiscard]] static VulkanBarrierInfo GetVulkanBarrierInfo(const RESOURCE_STATE _state);

		[[nodiscard]] static VkIndexType GetVulkanIndexType(const DATA_FORMAT _format);
		[[nodiscard]] static DATA_FORMAT GetRHIIndexType(const VkIndexType _type);

		[[nodiscard]] static VkPipelineBindPoint GetVulkanPipelineBindPoint(const PIPELINE_BIND_POINT _bindPoint);
		[[nodiscard]] static PIPELINE_BIND_POINT GetRHIPipelineBindPoint(const VkPipelineBindPoint _bindPoint);

		[[nodiscard]] static std::size_t GetVulkanQueueFamilyIndex(const COMMAND_TYPE _type, const VulkanDevice& _device);

		[[nodiscard]] static VkPrimitiveTopology GetVulkanTopology(const INPUT_TOPOLOGY _topology);
		[[nodiscard]] static INPUT_TOPOLOGY GetRHITopology(const VkPrimitiveTopology _topology);

		[[nodiscard]] static VkCullModeFlags GetVulkanCullMode(const CULL_MODE _mode);
		[[nodiscard]] static CULL_MODE GetRHICullMode(const VkCullModeFlags _mode);

		[[nodiscard]] static VkFrontFace GetVulkanWindingDirection(const WINDING_DIRECTION _direction);
		[[nodiscard]] static WINDING_DIRECTION GetRHIWindingDirection(const VkFrontFace _direction);

		[[nodiscard]] static VkCompareOp GetVulkanCompareOp(const COMPARE_OP _op);
		[[nodiscard]] static COMPARE_OP GetRHICompareOp(const VkCompareOp _op);

		[[nodiscard]] static VkStencilOp GetVulkanStencilOp(const STENCIL_OP _op);
		[[nodiscard]] static STENCIL_OP GetRHIStencilOp(const VkStencilOp _op);

		[[nodiscard]] static VkStencilOpState GetVulkanStencilOpState(const StencilOpState _state, const std::uint8_t _readMask, const std::uint8_t _writeMask);

		[[nodiscard]] static VkSampleCountFlagBits GetVulkanSampleCount(const SAMPLE_COUNT _count);
		[[nodiscard]] static SAMPLE_COUNT GetRHISampleCount(const VkSampleCountFlagBits _count);

		[[nodiscard]] static VkColorComponentFlags GetVulkanColourAspectFlags(const COLOUR_ASPECT_FLAGS _mask);
		[[nodiscard]] static COLOUR_ASPECT_FLAGS GetRHIColourAspectFlags(const VkColorComponentFlags _mask);

		[[nodiscard]] static VkBlendFactor GetVulkanBlendFactor(const BLEND_FACTOR _factor);
		[[nodiscard]] static BLEND_FACTOR GetRHIBlendFactor(const VkBlendFactor _factor);

		[[nodiscard]] static VkBlendOp GetVulkanBlendOp(const BLEND_OP _op);
		[[nodiscard]] static BLEND_OP GetRHIBlendOp(const VkBlendOp _op);

		[[nodiscard]] static VkLogicOp GetVulkanLogicOp(const LOGIC_OP _op);
		[[nodiscard]] static LOGIC_OP GetRHILogicOp(const VkLogicOp _op);

		[[nodiscard]] static VkFilter GetVulkanFilter(const FILTER_MODE _filterMode);
		[[nodiscard]] static FILTER_MODE GetRHIFilter(const VkFilter _filter);

		[[nodiscard]] static VkSamplerMipmapMode GetVulkanMipmapMode(const FILTER_MODE _filterMode);
		[[nodiscard]] static FILTER_MODE GetRHIMipmapMode(const VkSamplerMipmapMode _mipmapMode);

		[[nodiscard]] static VkSamplerAddressMode GetVulkanAddressMode(const ADDRESS_MODE _addressMode);
		[[nodiscard]] static ADDRESS_MODE GetRHIAddressMode(const VkSamplerAddressMode _addressMode);

		[[nodiscard]] static VkImageUsageFlags GetVulkanImageUsageFlags(const TEXTURE_USAGE_FLAGS _flags);
		[[nodiscard]] static TEXTURE_USAGE_FLAGS GetRHIImageUsageFlags(const VkImageUsageFlags _flags);

		[[nodiscard]] static VkFormat GetVulkanFormat(const DATA_FORMAT _format);
		[[nodiscard]] static DATA_FORMAT GetRHIFormat(const VkFormat _format);

		[[nodiscard]] static VkImageAspectFlags GetVulkanAspectFromRHIFormat(const DATA_FORMAT _format);

		[[nodiscard]] static VkImageViewType GetVulkanImageViewType(const TEXTURE_VIEW_DIMENSION _dimension);
		[[nodiscard]] static TEXTURE_VIEW_DIMENSION GetRHIImageViewDimension(const VkImageViewType _type);

		[[nodiscard]] static VkImageAspectFlags GetVulkanImageAspectFlags(const TEXTURE_ASPECT _aspect);
		[[nodiscard]] static TEXTURE_ASPECT GetRHIImageAspect(const VkImageAspectFlags _flags);
	};

}