#pragma once
#include "ICommandBuffer.h"
#include "IFence.h"
#include "ISemaphore.h"

namespace NK
{

	struct QueueDesc
	{
		COMMAND_POOL_TYPE type;
	};
	
	
	class IQueue
	{
	public:
		virtual ~IQueue() = default;

		virtual void Submit(ICommandBuffer* _cmdBuffer, ISemaphore* _waitSemaphore, ISemaphore* _signalSemaphore, IFence* _signalFence) = 0;
		virtual void WaitIdle() = 0;

		[[nodiscard]] inline COMMAND_POOL_TYPE GetType() const { return m_type; }


	protected:
		explicit IQueue(ILogger& _logger, IDevice& _device, const QueueDesc& _desc) : m_logger(_logger), m_device(_device), m_type(_desc.type) {}


		//Dependency injections
		ILogger& m_logger;
		IDevice& m_device;
		
		COMMAND_POOL_TYPE m_type;
	};

}