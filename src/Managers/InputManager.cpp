#include "InputManager.h"


namespace NK
{

	void InputManager::Update(Window* _window)
	{
		m_actionStatesLastUpdate = m_actionStates;

		GLFWwindow* glfwWin{ _window->GetGLFWWindow() };

		m_actionStates[INPUT_ACTION::MOVE_FORWARDS]		= (glfwGetKey(glfwWin, GLFW_KEY_W) == GLFW_PRESS) ? KEY_INPUT_STATE::HELD : KEY_INPUT_STATE::NOT_HELD;
		m_actionStates[INPUT_ACTION::MOVE_BACKWARDS]	= (glfwGetKey(glfwWin, GLFW_KEY_S) == GLFW_PRESS) ? KEY_INPUT_STATE::HELD : KEY_INPUT_STATE::NOT_HELD;
		m_actionStates[INPUT_ACTION::MOVE_LEFT]			= (glfwGetKey(glfwWin, GLFW_KEY_A) == GLFW_PRESS) ? KEY_INPUT_STATE::HELD : KEY_INPUT_STATE::NOT_HELD;
		m_actionStates[INPUT_ACTION::MOVE_RIGHT]		= (glfwGetKey(glfwWin, GLFW_KEY_D) == GLFW_PRESS) ? KEY_INPUT_STATE::HELD : KEY_INPUT_STATE::NOT_HELD;

		if (!m_firstUpdate)
		{
			double xPos;
			double yPos;
			glfwGetCursorPos(glfwWin, &xPos, &yPos);
			m_actionStates[INPUT_ACTION::CAMERA_YAW] = xPos;
			m_actionStates[INPUT_ACTION::CAMERA_PITCH] = yPos;
		}
		else
		{
			//Don't process mouse movement on first frame
			glfwSetInputMode(glfwWin, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			m_firstUpdate = false;
		}

		glfwSetWindowShouldClose(glfwWin, glfwGetKey(glfwWin, GLFW_KEY_ESCAPE) == GLFW_PRESS);
	}
	
}