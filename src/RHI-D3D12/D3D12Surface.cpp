#include "D3D12Surface.h"
#include <stdexcept>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#ifdef ERROR
	#undef ERROR
#endif

namespace NK
{

	D3D12Surface::D3D12Surface(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const SurfaceDesc& _desc)
	: ISurface(_logger, _allocator, _device, _desc)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::SURFACE, "Initialising D3D12Surface\n");

		glfwInit();
		m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::SURFACE, "GLFW initialised\n");

		CreateGLFWWindow();
		CreateSurface();

		m_logger.Unindent();
	}



	D3D12Surface::~D3D12Surface()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::SURFACE, "Shutting Down D3D12Surface\n");

		m_logger.Unindent();
	}



	void D3D12Surface::CreateGLFWWindow()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::SURFACE, "Creating window\n");

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); //don't let window be resized (for now)
		m_window = glfwCreateWindow(m_size.x, m_size.y, m_name.c_str(), nullptr, nullptr);
		if (m_window)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::SURFACE, "Window successfully created\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::SURFACE, "Failed to create window\n");
			throw std::runtime_error("");
		}

		m_logger.Unindent();
	}



	void D3D12Surface::CreateSurface()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::SURFACE, "Creating surface\n");

		m_surface = glfwGetWin32Window(m_window);

		m_logger.Unindent();
	}

}