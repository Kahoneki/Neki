#include "VulkanQueue.h"

#include "VulkanCommandBuffer.h"
#include "VulkanDevice.h"
#include "VulkanFence.h"
#include "VulkanSemaphore.h"

#include <Core/Utils/FormatUtils.h>

#include <stdexcept>


namespace NK
{

	VulkanQueue::VulkanQueue(ILogger& _logger, IDevice& _device, const QueueDesc& _desc, FreeListAllocator& _queueIndexAllocator)
	: IQueue(_logger, _device, _desc), m_queueIndexAllocator(_queueIndexAllocator)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::QUEUE, "Initialising VulkanQueue\n");


		//Allocate queue index
		m_queueIndex = m_queueIndexAllocator.Allocate();
		if (m_queueIndex == FreeListAllocator::INVALID_INDEX)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::QUEUE, "Max number of queues allocated for the provided queue family has been reached. Free them with NK_DELETE to make more.\n");
			throw std::runtime_error("");
		}

		
		//Get the queue
		VulkanDevice& vkDevice{ dynamic_cast<VulkanDevice&>(m_device) };
		switch (m_type)
		{
		case COMMAND_TYPE::GRAPHICS:
		{
			vkGetDeviceQueue(vkDevice.GetDevice(), vkDevice.GetGraphicsQueueFamilyIndex(), m_queueIndex, &m_queue);
			break;
		}
		case COMMAND_TYPE::COMPUTE:
		{
			vkGetDeviceQueue(vkDevice.GetDevice(), vkDevice.GetComputeQueueFamilyIndex(), m_queueIndex, &m_queue);
			break;
		}
		case COMMAND_TYPE::TRANSFER:
		{
			vkGetDeviceQueue(vkDevice.GetDevice(), vkDevice.GetTransferQueueFamilyIndex(), m_queueIndex, &m_queue);
			break;
		}
		}

		m_logger.Unindent();
	}



	VulkanQueue::~VulkanQueue()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::BUFFER, "Shutting Down VulkanQueue\n");


		if (m_queueIndex != FreeListAllocator::INVALID_INDEX)
		{
			m_queueIndexAllocator.Free(m_queueIndex);
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::QUEUE, "Queue Index Freed\n");
			m_queueIndex = FreeListAllocator::INVALID_INDEX;
		}
		

		m_logger.Unindent();
	}



	void VulkanQueue::Submit(ICommandBuffer* _cmdBuffer, ISemaphore* _waitSemaphore, ISemaphore* _signalSemaphore, IFence* _signalFence)
	{
		VkSemaphore vkWaitSemaphore{ _waitSemaphore ? dynamic_cast<VulkanSemaphore*>(_waitSemaphore)->GetSemaphore() : nullptr };
		VkPipelineStageFlags vkStageMask{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
		VkSemaphore vkSignalSemaphore{ _signalSemaphore ? dynamic_cast<VulkanSemaphore*>(_signalSemaphore)->GetSemaphore() : nullptr };
		VkFence vkSignalFence{ _signalFence ? dynamic_cast<VulkanFence*>(_signalFence)->GetFence() : VK_NULL_HANDLE };
		VkCommandBuffer vkCommandBuffer{ dynamic_cast<VulkanCommandBuffer*>(_cmdBuffer)->GetBuffer() };
		
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = _waitSemaphore ? 1 : 0;
		submitInfo.pWaitSemaphores = &vkWaitSemaphore;
		submitInfo.pWaitDstStageMask = &vkStageMask;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &vkCommandBuffer;
		submitInfo.signalSemaphoreCount = _signalSemaphore ? 1 : 0;
		submitInfo.pSignalSemaphores = &vkSignalSemaphore;

		const VkResult result{ vkQueueSubmit(m_queue, 1, &submitInfo, vkSignalFence) };
		if (result != VK_SUCCESS)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::QUEUE, "Failed to submit queue. result = " + std::to_string(result) + '\n');
			throw std::runtime_error("");
		}
	}



	void VulkanQueue::WaitIdle()
	{
		vkQueueWaitIdle(m_queue);
	}

}