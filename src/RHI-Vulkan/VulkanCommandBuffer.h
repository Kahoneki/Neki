#pragma once
#include "RHI/ICommandBuffer.h"

namespace NK
{
	class VulkanCommandBuffer final : public ICommandBuffer
	{
	public:
		explicit VulkanCommandBuffer(ILogger& _logger, IDevice& _device, ICommandPool& _pool, const CommandBufferDesc& _desc);
		virtual ~VulkanCommandBuffer() override;

		//todo: add command buffer methods here
		virtual void Reset() override;

		virtual void Begin() override;
		virtual void SetBlendConstants(const float _blendConstants[4]) override;
		virtual void End() override;
		
		//Vulkan internal API (for use by other RHI-Vulkan classes)
		[[nodiscard]] inline VkCommandBuffer GetBuffer() const { return m_buffer; }


	private:
		VkCommandBuffer m_buffer{ VK_NULL_HANDLE };
	};
}
