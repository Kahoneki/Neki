#include "Window.h"

#include <stdexcept>

namespace NK
{

	Window::Window(ILogger& _logger, const WindowDesc& _desc)
	: m_logger(_logger), m_name(_desc.name), m_size(_desc.size), m_window(nullptr)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::WINDOW, "Creating Window\n");

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); //don't let window be resized (for now)
		m_window = glfwCreateWindow(m_size.x, m_size.y, m_name.c_str(), nullptr, nullptr);
		if (m_window)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::WINDOW, "Window successfully created\n");
		}
		else
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::WINDOW, "Failed to create window\n");
			throw std::runtime_error("");
		}

		m_logger.Unindent();
	}



	Window::~Window()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::WINDOW, "Shutting Down Window\n");

		if (m_window != nullptr)
		{
			glfwDestroyWindow(m_window);
			m_window = nullptr;
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::WINDOW, "Window Destroyed\n");
		}

		m_logger.Unindent();
	}

}