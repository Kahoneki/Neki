#pragma once
#include <RHI/IPipeline.h>

namespace NK
{
	class VulkanPipeline final : public IPipeline
	{
	public:
		VulkanPipeline(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const PipelineDesc& _desc);
		virtual ~VulkanPipeline() override;

		//Vulkan internal API (for use by other RHI-Vulkan classes)
		[[nodiscard]] inline VkPipeline GetPipeline() const { return m_pipeline; }

		[[nodiscard]] static VkPrimitiveTopology GetVulkanTopology(INPUT_TOPOLOGY _topology);
		[[nodiscard]] static VkCullModeFlags GetVulkanCullMode(CULL_MODE _mode);
		[[nodiscard]] static VkFrontFace GetVulkanWindingDirection(WINDING_DIRECTION _direction);
		[[nodiscard]] static VkCompareOp GetVulkanCompareOp(COMPARE_OP _op);
		[[nodiscard]] static VkStencilOp GetVulkanStencilOp(STENCIL_OP _op);
		[[nodiscard]] static std::uint32_t Convert8BitMaskTo32BitMask(std::uint8_t _mask);
		[[nodiscard]] static VkStencilOpState GetVulkanStencilOpState(StencilOpState _state, std::uint8_t _readMask, std::uint8_t _writeMask);
		[[nodiscard]] static VkSampleCountFlagBits GetVulkanSampleCount(SAMPLE_COUNT _count);
		[[nodiscard]] static VkColorComponentFlags GetVulkanColourAspectFlags(COLOUR_ASPECT_FLAGS _mask);
		[[nodiscard]] static VkBlendFactor GetVulkanBlendFactor(BLEND_FACTOR _factor);
		[[nodiscard]] static VkBlendOp GetVulkanBlendOp(BLEND_OP _op);
		[[nodiscard]] static VkLogicOp GetVulkanLogicOp(LOGIC_OP _op);


	private:
		void CreateShaderModules(IShader* _compute, IShader* _vertex, IShader* _fragment);
		void CreateComputePipeline();
		void CreateGraphicsPipeline();
		
		[[nodiscard]] VkShaderModule CreateShaderModule(IShader* _shader) const;
		
		
		VkShaderModule m_computeShaderModule{ VK_NULL_HANDLE };
		VkShaderModule m_vertexShaderModule{ VK_NULL_HANDLE };
		VkShaderModule m_fragmentShaderModule{ VK_NULL_HANDLE };

		VkPipeline m_pipeline{ VK_NULL_HANDLE };
	};
}