#include "Window.h"

#include <Core/Context.h>

#include <stdexcept>


namespace NK
{

	Window::Window(const WindowDesc& _desc)
	: m_logger(Context::GetLogger()), m_name(_desc.name), m_size(_desc.size), m_window(nullptr)
	{
		m_logger->Indent();
		m_logger->Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::WINDOW, "Creating Window\n");

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); //don't let window be resized (for now)
		
		m_window = glfwCreateWindow(_desc.size.x, _desc.size.y, m_name.c_str(), nullptr, nullptr);
		if (m_window)
		{
			m_logger->IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::WINDOW, "Window successfully created\n");
		}
		else
		{
			m_logger->IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::WINDOW, "Failed to create window\n");
			throw std::runtime_error("");
		}
		
		int fbW, fbH, winW, winH;
		glfwGetFramebufferSize(m_window, &fbW, &fbH);
		glfwGetWindowSize(m_window, &winW, &winH);

		const float xRatio{ static_cast<float>(fbW) / winW };
		const float yRatio{ static_cast<float>(fbH) / winH };

		//resize window so the framebuffer matches the target pixels
		//e.g.: If target is 3840 and ratio is 3.5, window size becomes 1097
		const int targetWinW{ static_cast<int>(_desc.size.x / xRatio) };
		const int targetWinH{ static_cast<int>(_desc.size.y / yRatio) };
		glfwSetWindowSize(m_window, targetWinW, targetWinH);

		m_logger->Unindent();
	}



	Window::~Window()
	{
		m_logger->Indent();
		m_logger->Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::WINDOW, "Shutting Down Window\n");

		if (m_window != nullptr)
		{
			glfwDestroyWindow(m_window);
			m_window = nullptr;
			m_logger->IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::WINDOW, "Window Destroyed\n");
		}

		m_logger->Unindent();
	}



	void Window::SetCursorVisibility(const bool _visible) const
	{
		glfwSetInputMode(m_window, GLFW_CURSOR, _visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
	}

}