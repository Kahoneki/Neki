#pragma once
#include <RHI/IBufferView.h>


namespace NK
{

	class VulkanBufferView final : public IBufferView
	{
	public:
		explicit VulkanBufferView(ILogger& _logger, FreeListAllocator& _allocator, IDevice& _device, IBuffer* _buffer, const BufferViewDesc& _desc, VkDescriptorSet m_descriptorSet);
		~VulkanBufferView();
	};
	
}