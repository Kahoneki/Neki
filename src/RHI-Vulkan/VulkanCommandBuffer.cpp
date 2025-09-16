#include "VulkanCommandBuffer.h"

#include "VulkanCommandPool.h"
#include <stdexcept>

namespace NK
{

	VulkanCommandBuffer::VulkanCommandBuffer(ILogger& _logger, IDevice& _device, ICommandPool& _pool, const CommandBufferDesc& _desc)
	: ICommandBuffer(_logger, _device, _pool, _desc)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::COMMAND_POOL, "Initialising VulkanCommandBuffer\n");

		//Create the command buffer
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandBufferCount = 1;
		allocInfo.commandPool = dynamic_cast<VulkanCommandPool&>(m_pool).GetCommandPool();
		allocInfo.level = (m_level == COMMAND_BUFFER_LEVEL::PRIMARY ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY);
		const VkResult result{ vkAllocateCommandBuffers(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), &allocInfo, &m_buffer) };
		if (result == VK_SUCCESS)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::COMMAND_POOL, "Initialisation successful - level = " + std::string(m_level == COMMAND_BUFFER_LEVEL::PRIMARY ? "PRIMARY" : "SECONDARY") + '\n');
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::COMMAND_POOL, "Initialisation unsuccessful. result = " + std::to_string(result) + '\n');
			throw std::runtime_error("");
		}
		
		m_logger.Unindent();
	}



	VulkanCommandBuffer::~VulkanCommandBuffer()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::COMMAND_BUFFER, "Shutting Down VulkanCommandBuffer\n");
		
		if (m_buffer != VK_NULL_HANDLE)
		{
			vkFreeCommandBuffers(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), dynamic_cast<VulkanCommandPool&>(m_pool).GetCommandPool(), 1, &m_buffer);
			m_buffer = VK_NULL_HANDLE;
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::COMMAND_BUFFER, "Command Buffer Destroyed\n");
		}

		m_logger.Unindent();
	}



	void VulkanCommandBuffer::Reset()
	{
		//todo: implement
	}



	void VulkanCommandBuffer::Begin()
	{
		//todo: maybe add some of these flags as parameters
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		vkBeginCommandBuffer(m_buffer, &beginInfo);
	}



	void VulkanCommandBuffer::SetBlendConstants(const float _blendConstants[4])
	{
		vkCmdSetBlendConstants(m_buffer, _blendConstants);
	}



	void VulkanCommandBuffer::End()
	{
		vkEndCommandBuffer(m_buffer);
	}
}