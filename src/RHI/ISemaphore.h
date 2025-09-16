#pragma once
#include "IDevice.h"

namespace NK
{
	
	
	class ISemaphore
	{
	public:
		virtual ~ISemaphore() = default;


	protected:
		explicit ISemaphore(ILogger& _logger, IAllocator& _allocator, IDevice& _device)
		: m_logger(_logger), m_allocator(_allocator), m_device(_device) {}


		//Dependency injections
		ILogger& m_logger;
		IAllocator& m_allocator;
		IDevice& m_device;
	};

}