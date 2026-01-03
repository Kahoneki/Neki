#include "WindowLayer.h"

#include <Components/CWindow.h>
#include <Graphics/Window.h>


namespace NK
{

	WindowLayer::WindowLayer(Registry& _reg) : ILayer(_reg)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::WINDOW_LAYER, "Initialising Window Layer\n");
		
		m_logger.Unindent();
	}



	void WindowLayer::Update()
	{
		glfwPollEvents();
	}
	
}