#include "VulkanBuffer.h"

#include "VulkanDevice.h"
#include "VulkanUtils.h"

#include <Core/Utils/EnumUtils.h>
#include <Core/Utils/FormatUtils.h>

#include <stdexcept>



namespace NK
{

	VulkanBuffer::VulkanBuffer(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const BufferDesc& _desc)
	: IBuffer(_logger, _allocator, _device, _desc)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::BUFFER, "Initialising VulkanBuffer\n");

		//D3D12 requires CBVs are multiples of 256B. For safety, if the usage flags contains uniform buffer bit, round up to nearest multiple of 256B for parity with D3D12
		if (EnumUtils::Contains(m_usage, BUFFER_USAGE_FLAGS::UNIFORM_BUFFER_BIT))
		{
			if (m_size % 256 != 0)
			{
				m_size = (m_size + 255) & ~255; //Round up to nearest multiple of 256
				m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::BUFFER, "_desc.usage contained NK::BUFFER_USAGE_FLAGS::UNIFORM_BUFFER_BIT, but _desc.size (" + FormatUtils::GetSizeString(_desc.size) + ") is not a multiple of 256B as required for CBVs in D3D12. For parity and safety, all uniform buffers are required to be multiples of 256B. Rounding up to nearest multiple of 256B (" + FormatUtils::GetSizeString(m_size) + ")\n");
			}
		}


		//Create the buffer
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = m_size;
		bufferInfo.usage = VulkanUtils::GetVulkanBufferUsageFlags(m_usage);
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo{};
		if (m_memType == MEMORY_TYPE::DEVICE)
		{
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
			allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		}
		else
		{
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST; //Todo: VMA_MEMORY_USAGE_CPU_TO_GPU?
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
			allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		}
		VmaAllocationInfo allocResultInfo{};
		const VkResult result{ vmaCreateBuffer(dynamic_cast<VulkanDevice&>(m_device).GetVMAAllocator(), &bufferInfo, &allocInfo, &m_buffer, &m_allocation, &allocResultInfo) };
		if (result == VK_SUCCESS)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::BUFFER, "VkBuffer initialisation and allocation successful\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::BUFFER, "VkBuffer initialisation or allocation unsuccessful. result = " + std::to_string(result) + '\n');
			throw std::runtime_error("");
		}

		if (m_memType != MEMORY_TYPE::DEVICE)
		{
			m_map = allocResultInfo.pMappedData;
		}


		m_logger.Unindent();
	}



	VulkanBuffer::~VulkanBuffer()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::BUFFER, "Shutting Down VulkanBuffer\n");
		
		
		if (m_buffer != VK_NULL_HANDLE && m_allocation != VK_NULL_HANDLE)
		{
			vmaDestroyBuffer(dynamic_cast<VulkanDevice&>(m_device).GetVMAAllocator(), m_buffer, m_allocation);
			m_buffer = VK_NULL_HANDLE;
			m_allocation = VK_NULL_HANDLE;
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::BUFFER, "Buffer Destroyed And Allocation Freed\n");
		}
		if (m_map != nullptr)
		{
			m_map = nullptr;
		}

		
		m_logger.Unindent();
	}

}