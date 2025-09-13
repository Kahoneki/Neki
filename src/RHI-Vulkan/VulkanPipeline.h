#pragma once
#include <RHI/IPipeline.h>

namespace NK
{
	class VulkanPipeline final : public IPipeline
	{
	public:
		VulkanPipeline(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const PipelineDesc& _desc);
		virtual ~VulkanPipeline() override;


	private:
		void CreateShaderModules(IShader* _compute, IShader* _vertex, IShader* _fragment);
		void CreateComputePipeline();
		void CreateGraphicsPipeline();
		
		[[nodiscard]] VkShaderModule CreateShaderModule(IShader* _shader) const;

		[[nodiscard]] static VkPrimitiveTopology GetVulkanTopology(INPUT_TOPOLOGY _topology);
		[[nodiscard]] static VkSampleCountFlagBits GetVulkanSampleCount(SAMPLE_COUNT _count);
		[[nodiscard]] static VkFlags GetVulkanColourAspectFlags(COLOUR_ASPECT_FLAGS _mask);
		[[nodiscard]] static VkBlendFactor GetVulkanBlendFactor(BLEND_FACTOR _factor);
		[[nodiscard]] static VkBlendOp GetVulkanBlendOp(BLEND_OP _op);
		[[nodiscard]] static VkLogicOp GetVulkanLogicOp(LOGIC_OP _op);
		
		
		VkShaderModule m_computeShaderModule{ VK_NULL_HANDLE };
		VkShaderModule m_vertexShaderModule{ VK_NULL_HANDLE };
		VkShaderModule m_fragmentShaderModule{ VK_NULL_HANDLE };

		VkPipeline m_pipeline{ VK_NULL_HANDLE };
	};
}