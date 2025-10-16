#pragma once

#include <Core/Debug/ILogger.h>

#include <string>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>


namespace NK
{
	
	struct WindowDesc
	{
		std::string name; //Name of the window
		glm::ivec2 size; //Size of the window in pixels
	};
	

	//Wrapper for a GLFWwindow
	class Window final
	{
	public:
		explicit Window(ILogger& _logger, const WindowDesc& _desc);
		~Window();

		[[nodiscard]] inline glm::ivec2 GetSize() const { return m_size; }
		[[nodiscard]] inline bool ShouldClose() const { return glfwWindowShouldClose(m_window); }
		[[nodiscard]] inline GLFWwindow* GetGLFWWindow() const { return m_window; }
		
		
	protected:
		//Dependency injections
		ILogger& m_logger;

		const std::string m_name;
		const glm::ivec2 m_size; //const for now O_O

		GLFWwindow* m_window;
	};
	
}