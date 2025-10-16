#pragma once

#include <RHI/IBuffer.h>


namespace NK
{
	
	class VulkanBuffer final : public IBuffer
	{
	public:
		explicit VulkanBuffer(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const BufferDesc& _desc);
		virtual ~VulkanBuffer() override;

		virtual void* Map() override;
		virtual void Unmap() override;

		//Vulkan internal API (for use by other RHI-Vulkan classes)
		[[nodiscard]] inline VkBuffer GetBuffer() const { return m_buffer; }


	private:
		VkBuffer m_buffer{ VK_NULL_HANDLE };
		VkDeviceMemory m_memory{ VK_NULL_HANDLE };
	};
	
}