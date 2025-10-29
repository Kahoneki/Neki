#pragma once

#include <RHI/IBuffer.h>


namespace NK
{
	
	class VulkanBuffer final : public IBuffer
	{
	public:
		explicit VulkanBuffer(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const BufferDesc& _desc);
		virtual ~VulkanBuffer() override;

		[[nodiscard]] virtual void* GetMap() override;

		//Vulkan internal API (for use by other RHI-Vulkan classes)
		[[nodiscard]] inline VkBuffer GetBuffer() const { return m_buffer; }


	private:
		VkBuffer m_buffer{ VK_NULL_HANDLE };
		VmaAllocation m_allocation{ VK_NULL_HANDLE };
		void* m_map{ nullptr };
	};
	
}