#pragma once
#include "VulkanDevice.h"
#include "RHI/ICommandPool.h"

namespace NK
{
	class VulkanCommandPool final : public ICommandPool
	{
	public:
		explicit VulkanCommandPool(ILogger& _logger, IAllocator& _allocator, VulkanDevice& _device, const CommandPoolDesc& _desc);
		virtual ~VulkanCommandPool() override;
		
		//ICommandPool interface implementation
		[[nodiscard]] virtual UniquePtr<ICommandBuffer> AllocateCommandBuffer(const CommandBufferDesc& _desc) override;
		virtual void Reset(COMMAND_POOL_RESET_FLAGS _type) override;

		//Vulkan internal API (for use by other RHI-Vulkan classes)
		[[nodiscard]] inline VkCommandPool GetCommandPool() const { return m_pool; }


	private:
		[[nodiscard]] std::size_t GetQueueFamilyIndex() const;
		[[nodiscard]] std::string GetPoolTypeString() const;
		
		VkCommandPool m_pool{ VK_NULL_HANDLE };
	};
}