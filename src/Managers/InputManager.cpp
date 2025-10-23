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



	INPUT_BINDING_TYPE InputManager::GetActionInputType(const ActionTypeMapKey& _key)
	{
		return (m_actionToInputTypeMap.contains(_key) ? m_actionToInputTypeMap[_key] : INPUT_BINDING_TYPE::UNBOUND);
	}



	ButtonState InputManager::GetButtonState(const ActionTypeMapKey& _key)
	{
		const ButtonBinding& binding{ std::get<ButtonBinding>(m_actionToInputMap[_key]) };
		return EvaluateButtonBinding(binding);
	}



	Axis1DState InputManager::GetAxis1DState(const ActionTypeMapKey& _key)
	{
		const Axis1DBinding& binding{ std::get<Axis1DBinding>(m_actionToInputMap[_key]) };
		return EvaluateAxis1DBinding(binding);
	}



	Axis2DState InputManager::GetAxis2DState(const ActionTypeMapKey& _key)
	{
		const Axis2DBinding& binding{ std::get<Axis2DBinding>(m_actionToInputMap[_key]) };
		return EvaluateAxis2DBinding(binding);
	}



	void InputManager::ValidateActionTypeUtil(const std::string& _func, const ActionTypeMapKey& _key, const INPUT_BINDING_TYPE _inputType)
	{
		if (m_actionToInputTypeMap[_key] != _inputType)
		{
			std::string err{ "InputManager::" };
			err += _func + " - invalid action provided - type: " + std::string(_key.first.name()) + ", element: " + std::to_string(_key.second);
			throw std::invalid_argument(err);
		}
	}



	ButtonState InputManager::EvaluateButtonBinding(const ButtonBinding& _binding)
	{
		ButtonState state{};
		switch (_binding.type)
		{
		case INPUT_VARIANT_ENUM_TYPE::NONE:
		{
			break;
		}
		case INPUT_VARIANT_ENUM_TYPE::KEYBOARD:
		{
			state.held = GetKeyPressed(std::get<KEYBOARD>(_binding.input));
			state.released = GetKeyReleased(std::get<KEYBOARD>(_binding.input));
			break;
		}
		case INPUT_VARIANT_ENUM_TYPE::MOUSE_BUTTON:
		{
			state.held = GetMouseButtonPressed(std::get<MOUSE_BUTTON>(_binding.input));
			state.released = GetMouseButtonReleased(std::get<MOUSE_BUTTON>(_binding.input));
			break;
		}
		default:
		{
			throw std::runtime_error("Default case reached for InputManager::GetButtonState() - this indicates an internal error, please make a GitHub issue on the topic");
		}
		}

		return state;
	}



	Axis1DState InputManager::EvaluateAxis1DBinding(const Axis1DBinding& _binding)
	{
		if (_binding.digital)
		{
			const ButtonState bs1{ EvaluateButtonBinding(_binding.buttonBindings.first) };
			const ButtonState bs2{ EvaluateButtonBinding(_binding.buttonBindings.second) };

			const float value{ (bs1.held * _binding.digitalValues.first) + (bs2.held * _binding.digitalValues.second) };
			return Axis1DState(value);
		}

		//Native Axis1DBinding - none currently supported
		Axis1DState state{};
		switch (_binding.type)
		{
		default:
		{
			throw std::runtime_error("Default case reached for Axis1DInput::GetState() - this indicates an internal error, please make a GitHub issue on the topic");
		}
		}

		return state;
	}



	Axis2DState InputManager::EvaluateAxis2DBinding(const Axis2DBinding& _binding)
	{
		if (_binding.doubleAxes)
		{
			const Axis1DState as1{ EvaluateAxis1DBinding(_binding.axis1DBindings.first) };
			const Axis1DState as2{ EvaluateAxis1DBinding(_binding.axis1DBindings.second) };

			return Axis2DState({ as1.value, as2.value });
		}


		//Native Axis2DBinding
		switch (_binding.type)
		{
		case (INPUT_VARIANT_ENUM_TYPE::MOUSE):
		{
			const MOUSE mouseInputType{ std::get<MOUSE>(_binding.input) };
			if (mouseInputType == MOUSE::POSITION)
			{
				return Axis2DState(GetMousePosition());
			}
			if (mouseInputType == MOUSE::POSITION_DIFFERENCE)
			{
				return Axis2DState(GetMouseDiff());
			}
			throw std::runtime_error("mouseInputType not recognised in Axis2DInput::GetState() - this indicates an internal error, please make a GitHub issue on the topic");
		}
		default:
		{
			throw std::runtime_error("Default case reached for Axis2DInput::GetState() - this indicates an internal error, please make a GitHub issue on the topic");
		}
		}
	}

}