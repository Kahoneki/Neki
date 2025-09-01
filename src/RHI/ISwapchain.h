#pragma once
#include "IDevice.h"
#include "ISurface.h"

namespace NK
{


	struct SwapchainDesc
	{
		ISurface* surface;
		std::uint32_t numBuffers;
	};
	
	
	class ISwapchain
	{
	public:
		virtual ~ISwapchain() = default;


	protected:
		explicit ISwapchain(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const SwapchainDesc& _desc)
		: m_logger(_logger), m_allocator(_allocator), m_device(_device), m_surface(_desc.surface), m_numBuffers(_desc.numBuffers) {}
		
		//Dependency injections
		ILogger& m_logger;
		IAllocator& m_allocator;
		IDevice& m_device;

		ISurface* m_surface;
		std::uint32_t m_numBuffers;

		VkExtent2D m_extent{ 0,0 };
	};
	
}