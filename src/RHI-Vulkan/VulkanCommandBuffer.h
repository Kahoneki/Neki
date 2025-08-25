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


	private:
		VkCommandBuffer m_buffer{ VK_NULL_HANDLE };
	};
}
