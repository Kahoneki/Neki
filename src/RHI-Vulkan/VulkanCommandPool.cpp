#include "VulkanCommandPool.h"

#include "VulkanCommandBuffer.h"
#include "VulkanUtils.h"

#include <Core/Memory/Allocation.h>
#include <RHI/RHIUtils.h>

#include <stdexcept>
#include <utility>




namespace NK
{

	VulkanCommandPool::VulkanCommandPool(ILogger& _logger, IAllocator& _allocator, VulkanDevice& _device, const CommandPoolDesc& _desc)
	: ICommandPool(_logger, _allocator, _device, _desc)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::COMMAND_POOL, "Initialising VulkanCommandPool\n");

		//Create the pool
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; //Allow individual command buffers to be reset
		poolInfo.queueFamilyIndex = VulkanUtils::GetVulkanQueueFamilyIndex(m_type, dynamic_cast<VulkanDevice&>(m_device));
		const VkResult result{ vkCreateCommandPool(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), &poolInfo, _allocator.GetVulkanCallbacks(), &m_pool) };
		if (result == VK_SUCCESS)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::COMMAND_POOL, "Initialisation successful - type = " + RHIUtils::GetCommandTypeString(m_type) + '\n');
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_POOL, "Initialisation unsuccessful. result = " + std::to_string(result) + '\n');
			throw std::runtime_error("");
		}

		m_logger.Unindent();
	}



	VulkanCommandPool::~VulkanCommandPool()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::COMMAND_POOL, "Shutting Down VulkanCommandPool\n");
		
		if (m_pool != VK_NULL_HANDLE)
		{
			vkDestroyCommandPool(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), m_pool, m_allocator.GetVulkanCallbacks());
			m_pool = VK_NULL_HANDLE;
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::COMMAND_POOL, "Command Pool (and all associated command buffers) Destroyed\n");
		}

		m_logger.Unindent();
	}



	UniquePtr<ICommandBuffer> VulkanCommandPool::AllocateCommandBuffer(const CommandBufferDesc& _desc)
	{
		return UniquePtr<ICommandBuffer>(NK_NEW(VulkanCommandBuffer, m_logger, m_device, *this, _desc));
	}



	void VulkanCommandPool::Reset(COMMAND_POOL_RESET_FLAGS _type)
	{
		//todo: implement
	}
	
}