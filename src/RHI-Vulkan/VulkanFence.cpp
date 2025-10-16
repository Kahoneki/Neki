#include "VulkanFence.h"

#include "VulkanDevice.h"

#include <Core/Utils/FormatUtils.h>

#include <stdexcept>


namespace NK
{

	VulkanFence::VulkanFence(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const FenceDesc& _desc)
	: IFence(_logger, _allocator, _device, _desc)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::FENCE, "Initialising VulkanFence\n");

		//Create the fence
		VkFenceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		createInfo.flags = _desc.initiallySignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
		const VkResult result{ vkCreateFence(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), &createInfo, m_allocator.GetVulkanCallbacks(), &m_fence) };
		if (result == VK_SUCCESS)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::FENCE, "VkFence initialisation successful\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::FENCE, "VkFence initialisation unsuccessful. result = " + std::to_string(result) + '\n');
			throw std::runtime_error("");
		}


		m_logger.Unindent();
	}



	VulkanFence::~VulkanFence()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::FENCE, "Shutting Down VulkanFence\n");

		
		if (m_fence != VK_NULL_HANDLE)
		{
			vkDestroyFence(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), m_fence, m_allocator.GetVulkanCallbacks());
			m_fence = VK_NULL_HANDLE;
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::FENCE, "Fence Destroyed\n");
		}

		
		m_logger.Unindent();
	}



	void VulkanFence::Wait()
	{
		vkWaitForFences(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), 1, &m_fence, VK_TRUE, UINT64_MAX);
	}



	void VulkanFence::Reset()
	{
		vkResetFences(dynamic_cast<VulkanDevice&>(m_device).GetDevice(), 1, &m_fence);
	}
	
}