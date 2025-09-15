#include "VulkanSemaphore.h"
#include <stdexcept>
#include <Core/Utils/FormatUtils.h>
#include "VulkanDevice.h"

namespace NK
{

	VulkanSemaphore::VulkanSemaphore(ILogger& _logger, IAllocator& _allocator, IDevice& _device)
	: ISemaphore(_logger, _allocator, _device)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::SEMAPHORE, "Initialising VulkanSemaphore\n");

		//Create the semaphore
		VkSemaphoreCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		const VkResult result{ vkCreateSemaphore(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), &createInfo, m_allocator.GetVulkanCallbacks(), &m_semaphore) };
		if (result == VK_SUCCESS)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::SEMAPHORE, "VkSemaphore initialisation successful\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::SEMAPHORE, "VkSemaphore initialisation unsuccessful. result = " + std::to_string(result) + '\n');
			throw std::runtime_error("");
		}


		m_logger.Unindent();
	}



	VulkanSemaphore::~VulkanSemaphore()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::SEMAPHORE, "Shutting Down VulkanSemaphore\n");

		
		if (m_semaphore != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), m_semaphore, m_allocator.GetVulkanCallbacks());
			m_semaphore = VK_NULL_HANDLE;
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::SEMAPHORE, "Semaphore Destroyed\n");
		}

		
		m_logger.Unindent();
	}
	
}