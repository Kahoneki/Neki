#include "VulkanBufferView.h"
#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include <stdexcept>

namespace NK
{

	VulkanBufferView::VulkanBufferView(ILogger& _logger, FreeListAllocator& _allocator, IDevice& _device, IBuffer* _buffer, const BufferViewDesc& _desc, VkDescriptorSet m_descriptorSet)
	: IBufferView(_logger, _allocator, _device, _buffer, _desc)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::BUFFER_VIEW, "Creating buffer view\n");

		
		//Get resource index from free list
		m_resourceIndex = m_allocator.Allocate();
		if (m_resourceIndex == FreeListAllocator::INVALID_INDEX)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::BUFFER_VIEW, "Resource index allocation failed - max bindless resource count (" + std::to_string(MAX_BINDLESS_RESOURCES) + ") reached.\n");
			throw std::runtime_error("");
		}


		//Get underlying VulkanBuffer
		const VulkanBuffer* vulkanBuffer{ dynamic_cast<const VulkanBuffer*>(_buffer) };
		if (!vulkanBuffer)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::BUFFER_VIEW, "Dynamic cast to VulkanBuffer object expected to pass but failed\n");
			throw std::runtime_error("");
		}


		//Convert rhi view type to vulkan descriptor type
		VkDescriptorType descriptorType;
		switch (_desc.type)
		{
		case BUFFER_VIEW_TYPE::UNIFORM:
			descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			break;

		case BUFFER_VIEW_TYPE::STORAGE_READ_ONLY:
		case BUFFER_VIEW_TYPE::STORAGE_READ_WRITE:
			descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			break;

		default:
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::BUFFER_VIEW, "Default case reached when determining vulkan descriptor type\n");
			throw std::runtime_error("");
		}


		//Populate descriptor info
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = vulkanBuffer->GetBuffer();
		bufferInfo.offset = _desc.offset;
		bufferInfo.range = _desc.size;


		//Populate write info
		VkWriteDescriptorSet writeInfo{};
		writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeInfo.dstSet = m_descriptorSet;
		writeInfo.dstBinding = 0; //All bindless resource views are in binding point 0
		writeInfo.dstArrayElement = m_resourceIndex;
		writeInfo.descriptorCount = 1;
		writeInfo.descriptorType = descriptorType;
		writeInfo.pBufferInfo = &bufferInfo;


		vkUpdateDescriptorSets(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), 1, &writeInfo, 0, nullptr);
		

		m_logger.Unindent();
	}



	VulkanBufferView::~VulkanBufferView()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::BUFFER_VIEW, "Shutting Down VulkanBufferView\n");

		if (m_resourceIndex != FreeListAllocator::INVALID_INDEX)
		{
			m_allocator.Free(m_resourceIndex);
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::BUFFER_VIEW, "Resource Index Freed\n");
			m_resourceIndex = FreeListAllocator::INVALID_INDEX;
		}
		
		m_logger.Unindent();
	}
	
}