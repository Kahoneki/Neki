#include "InputManager.h"


namespace NK
{


	void InputManager::UpdateMouse()
	{
		if (!m_window) { throw std::runtime_error("InputManager::UpdateMouse() was called, but m_window is nullptr. Set the window with InputManager::SetWindow()"); }

		if (m_firstUpdate)
		{
			//Don't process mouse movement on the first update, it creates a large delta
			m_firstUpdate = false;
			return;
		}
		double x, y;
		glfwGetCursorPos(m_window->GetGLFWWindow(), &x, &y);
		m_mousePosLastFrame = m_mousePosThisFrame;
		m_mousePosThisFrame = { x, y };
	}



	bool InputManager::GetKeyPressed(const KEYBOARD _key)
	{
		if (!m_window) { throw std::runtime_error("InputManager::GetKeyHeld() was called internally, but m_window is nullptr. Set the window with InputManager::SetWindow() before the first InputLayer::Update()"); }
		return (glfwGetKey(m_window->GetGLFWWindow(), InputUtils::GetGLFWKeyboardKey(_key)) == GLFW_PRESS);
	}



	bool InputManager::GetKeyReleased(const KEYBOARD _key)
	{
		if (!m_window) { throw std::runtime_error("InputManager::GetKeyReleased() was called internally, but m_window is nullptr. Set the window with InputManager::SetWindow() before the first InputLayer::Update()"); }
		return (glfwGetKey(m_window->GetGLFWWindow(), InputUtils::GetGLFWKeyboardKey(_key)) == GLFW_RELEASE);
	}



	bool InputManager::GetMouseButtonPressed(const MOUSE_BUTTON _button)
	{
		if (!m_window) { throw std::runtime_error("InputManager::GetMouseButtonHeld() was called internally, but m_window is nullptr. Set the window with InputManager::SetWindow() before the first InputLayer::Update()"); }
		return (glfwGetKey(m_window->GetGLFWWindow(), InputUtils::GetGLFWMouseButton(_button)) == GLFW_PRESS);
	}



	bool InputManager::GetMouseButtonReleased(const MOUSE_BUTTON _button)
	{
		if (!m_window) { throw std::runtime_error("InputManager::GetMouseButtonReleased() was called internally, but m_window is nullptr. Set the window with InputManager::SetWindow() before the first InputLayer::Update()"); }
		return (glfwGetKey(m_window->GetGLFWWindow(), InputUtils::GetGLFWMouseButton(_button)) == GLFW_RELEASE);
	}



	glm::vec2 InputManager::GetMouseDiff()
	{
		if (m_firstUpdate) { return { 0, 0 }; }
		return m_mousePosThisFrame - m_mousePosLastFrame;
	}



	glm::vec2 InputManager::GetMousePosition()
	{
		return m_mousePosThisFrame;
	}



	INPUT_TYPE InputManager::GetActionInputType(const ActionTypeMapKey& _key)
	{
		return (m_actionToInputTypeMap.contains(_key) ? m_actionToInputTypeMap[_key] : INPUT_TYPE::UNBOUND);
	}



	ButtonState InputManager::GetButtonState(const ActionTypeMapKey& _key)
	{
		ValidateActionTypeUtil("GetButtonState()", _key, INPUT_TYPE::BUTTON);
		return dynamic_cast<const ButtonInput* const>(m_actionToInputMap[_key])->GetState();
	}



	Axis1DState InputManager::GetAxis1DState(const ActionTypeMapKey& _key)
	{
		ValidateActionTypeUtil("GetAxis1DState()", _key, INPUT_TYPE::AXIS_1D);
		return dynamic_cast<const Axis1DInput* const>(m_actionToInputMap[_key])->GetState();
	}



	Axis2DState InputManager::GetAxis2DState(const ActionTypeMapKey& _key)
	{
		ValidateActionTypeUtil("GetAxis2DState()", _key, INPUT_TYPE::AXIS_2D);
		return dynamic_cast<const Axis2DInput* const>(m_actionToInputMap[_key])->GetState();
	}



	void InputManager::ValidateActionTypeUtil(const std::string& _func, const ActionTypeMapKey& _key, const INPUT_TYPE _inputType)
	{
		if (m_actionToInputTypeMap[_key] != _inputType)
		{
			std::string err{ "InputManager::" };
			err += _func + " - invalid action provided - type: " + std::string(_key.first.name()) + ", element: " + std::to_string(_key.second);
			throw std::invalid_argument(err);
		}
	}

}