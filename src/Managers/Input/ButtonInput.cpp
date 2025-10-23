#include "ButtonInput.h"

#include <Managers/InputManager.h>


namespace NK
{

	ButtonState ButtonInput::GetState() const
	{
		ButtonState state{};

		switch (binding.type)
		{
		case INPUT_VARIANT_ENUM_TYPE::NONE:
		{
			break;
		}
		case INPUT_VARIANT_ENUM_TYPE::KEYBOARD:
		{
			state.held = InputManager::GetKeyPressed(std::get<KEYBOARD>(binding.input));
			state.released = InputManager::GetKeyReleased(std::get<KEYBOARD>(binding.input));
			break;
		}
		case INPUT_VARIANT_ENUM_TYPE::MOUSE_BUTTON:
		{
			state.held = InputManager::GetMouseButtonPressed(std::get<MOUSE_BUTTON>(binding.input));
			state.released = InputManager::GetMouseButtonReleased(std::get<MOUSE_BUTTON>(binding.input));
			break;
		}
		default:
		{
			throw std::runtime_error("Default case reached for ButtonInput::GetState() - this indicates an internal error, please make a GitHub issue on the topic");
		}
		}

		return state;
	}
	
}