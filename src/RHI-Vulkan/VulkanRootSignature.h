#pragma once
#include <RHI/IRootSignature.h>

namespace NK
{
	class VulkanRootSignature final : public IRootSignature
	{
	public:
		explicit VulkanRootSignature(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const RootSignatureDesc& _desc);
		virtual ~VulkanRootSignature() override;

		//Vulkan internal API (for use by other RHI-Vulkan classes)
		[[nodiscard]] inline VkDescriptorSet GetDescriptorSet() const { return m_descriptorSet; }
		[[nodiscard]] inline VkPipelineLayout GetPipelineLayout() const { return m_pipelineLayout; }
		

	private:
		VkDescriptorSet m_descriptorSet{ VK_NULL_HANDLE };
		VkPipelineLayout m_pipelineLayout{ VK_NULL_HANDLE };
	};
}