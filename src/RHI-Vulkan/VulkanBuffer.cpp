#include "VulkanBuffer.h"

#include "VulkanDevice.h"
#include "Core/Utils/FormatUtils.h"

namespace NK
{

	VulkanBuffer::VulkanBuffer(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const BufferDesc& _desc)
	: IBuffer(_logger, _allocator, _device, _desc)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::BUFFER, "Initialising VulkanBuffer\n");


		//Create the buffer
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = m_size;
		bufferInfo.usage = GetVulkanUsageFlags();
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VkResult result{ vkCreateBuffer(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), &bufferInfo, m_allocator.GetVulkanCallbacks(), &m_buffer) };
		if (result == VK_SUCCESS)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::BUFFER, "VkBuffer initialisation successful\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::BUFFER, "VkBuffer initialisation unsuccessful. result = " + std::to_string(result) + '\n');
			throw std::runtime_error("");
		}


		//Search for a memory type that satisfies buffer's internal requirements and m_memType
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), m_buffer, &memRequirements);

		VkMemoryPropertyFlags memProps;
		switch (m_memType)
		{
		case MEMORY_TYPE::HOST: memProps = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			break;
		case MEMORY_TYPE::DEVICE: memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			break;
		default:
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::BUFFER, "Provided _desc.type (" + std::to_string(std::to_underlying(m_memType)) + ") not supported\n");
			throw std::runtime_error("");
		}
		}

		VkPhysicalDeviceMemoryProperties physicalDeviceMemProps;
		vkGetPhysicalDeviceMemoryProperties(dynamic_cast<VulkanDevice&>(m_device).GetPhysicalDevice(), &physicalDeviceMemProps);
		std::uint32_t memTypeIndex{ UINT32_MAX };
		for (std::uint32_t i{ 0 }; i < physicalDeviceMemProps.memoryTypeCount; ++i)
		{
			//Check if memory type is allowed for the buffer
			if (!(memRequirements.memoryTypeBits & (1 << i))) { continue; }
			if ((physicalDeviceMemProps.memoryTypes[i].propertyFlags & memProps) != memProps) { continue; }

			//This memory type is allowed
			memTypeIndex = i;
			break;
		}
		if (memTypeIndex == UINT32_MAX)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::BUFFER, "No suitable memory type could be found\n");
			throw std::runtime_error("");
		}


		//Allocate memory for the buffer
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = memTypeIndex;
		result = vkAllocateMemory(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), &allocInfo, m_allocator.GetVulkanCallbacks(), &m_memory);
		if (result == VK_SUCCESS)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::BUFFER, "VkDeviceMemory allocation successful (" + FormatUtils::GetSizeString(memRequirements.size) + ")\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::BUFFER, "VkDeviceMemory allocation unsuccessful. result = " + std::to_string(result) + '\n');
			throw std::runtime_error("");
		}


		//Bind the allocated memory region to the buffer
		result = vkBindBufferMemory(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), m_buffer, m_memory, 0);
		if (result == VK_SUCCESS)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::BUFFER, "Memory binding successful\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::BUFFER, "Memory binding unsuccessful. result = " + std::to_string(result) + '\n');
			throw std::runtime_error("");
		}


		m_logger.Unindent();
	}



	VulkanBuffer::~VulkanBuffer()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::BUFFER, "Shutting Down VulkanBuffer\n");

		if (m_memory != VK_NULL_HANDLE)
		{
			vkFreeMemory(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), m_memory, m_allocator.GetVulkanCallbacks());
			m_memory = VK_NULL_HANDLE;
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::BUFFER, "Buffer Memory Freed\n");
		}
		
		if (m_buffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), m_buffer, m_allocator.GetVulkanCallbacks());
			m_buffer = VK_NULL_HANDLE;
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::BUFFER, "Buffer Destroyed\n");
		}

		m_logger.Unindent();
	}



	void* VulkanBuffer::Map()
	{
		//todo: implement
		return nullptr;
	}



	void VulkanBuffer::Unmap()
	{
		//todo: implement
	}



	VkBufferUsageFlags VulkanBuffer::GetVulkanUsageFlags() const
	{
		VkBufferUsageFlags vkFlags{ 0 };
		
		if (m_usage & std::to_underlying(BUFFER_USAGE_FLAG_BITS::TRANSFER_SRC_BIT))		{ vkFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT; }
		if (m_usage & std::to_underlying(BUFFER_USAGE_FLAG_BITS::TRANSFER_DST_BIT))		{ vkFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT; }
		if (m_usage & std::to_underlying(BUFFER_USAGE_FLAG_BITS::UNIFORM_BUFFER_BIT))	{ vkFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT; }
		if (m_usage & std::to_underlying(BUFFER_USAGE_FLAG_BITS::STORAGE_BUFFER_BIT))	{ vkFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT; }
		if (m_usage & std::to_underlying(BUFFER_USAGE_FLAG_BITS::VERTEX_BUFFER_BIT))	{ vkFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; }
		if (m_usage & std::to_underlying(BUFFER_USAGE_FLAG_BITS::INDEX_BUFFER_BIT))		{ vkFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT; }
		if (m_usage & std::to_underlying(BUFFER_USAGE_FLAG_BITS::INDIRECT_BUFFER_BIT))	{ vkFlags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT; }

		return vkFlags;
	}

}