#pragma once

#include "IDevice.h"


namespace NK
{

	struct FenceDesc
	{
		bool initiallySignaled;
	};
	
	
	class IFence
	{
	public:
		virtual ~IFence() = default;

		virtual void Wait() = 0;
		virtual void Reset() = 0;


	protected:
		explicit IFence(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const FenceDesc& _desc)
		: m_logger(_logger), m_allocator(_allocator), m_device(_device) {}


		//Dependency injections
		ILogger& m_logger;
		IAllocator& m_allocator;
		IDevice& m_device;
	};

}