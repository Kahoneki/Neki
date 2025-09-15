#include "VulkanQueue.h"
#include <stdexcept>
#include <Core/Utils/FormatUtils.h>
#include "VulkanDevice.h"

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
		case QUEUE_TYPE::GRAPHICS:
		{
			vkGetDeviceQueue(vkDevice.GetDevice(), vkDevice.GetGraphicsQueueFamilyIndex(), m_queueIndex, &m_queue);
			break;
		}
		case QUEUE_TYPE::COMPUTE:
		{
			vkGetDeviceQueue(vkDevice.GetDevice(), vkDevice.GetComputeQueueFamilyIndex(), m_queueIndex, &m_queue);
			break;
		}
		case QUEUE_TYPE::TRANSFER:
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
		//todo: implement
	}

}