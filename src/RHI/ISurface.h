#pragma once

#include "IDevice.h"

#include <Graphics/Window.h>


namespace NK
{
	
	class ISurface
	{
	public:
		virtual ~ISurface() = default;

		[[nodiscard]] inline Window* GetWindow() const { return m_window; }
		
		
	protected:
		explicit ISurface(ILogger& _logger, IAllocator& _allocator, IDevice& _device, Window* _window)
		: m_logger(_logger), m_allocator(_allocator), m_device(_device), m_window(_window) {}
		
		//Dependency injections
		ILogger& m_logger;
		IAllocator& m_allocator;
		IDevice& m_device;
		
		Window* m_window;
	};
	
}