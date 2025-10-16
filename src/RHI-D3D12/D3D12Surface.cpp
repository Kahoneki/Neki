#include "D3D12Surface.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>


namespace NK
{

	D3D12Surface::D3D12Surface(ILogger& _logger, IAllocator& _allocator, IDevice& _device, Window* _window)
	: ISurface(_logger, _allocator, _device, _window)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::SURFACE, "Initialising D3D12Surface\n");

		CreateSurface();

		m_logger.Unindent();
	}



	D3D12Surface::~D3D12Surface()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::SURFACE, "Shutting Down D3D12Surface\n");

		m_logger.Unindent();
	}



	void D3D12Surface::CreateSurface()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::SURFACE, "Creating surface\n");

		m_surface = glfwGetWin32Window(m_window->GetGLFWWindow());

		m_logger.Unindent();
	}

}