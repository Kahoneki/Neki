#include "InputUtils.h"

#include <GLFW/glfw3.h>
#include <stdexcept>
#include <string>


namespace NK
{

	INPUT_VARIANT_ENUM_TYPE InputUtils::GetInputType(const INPUT_VARIANT _input)
	{
		if (std::holds_alternative<KEYBOARD>(_input))		{ return INPUT_VARIANT_ENUM_TYPE::KEYBOARD; }
		if (std::holds_alternative<MOUSE_BUTTON>(_input))	{ return INPUT_VARIANT_ENUM_TYPE::MOUSE_BUTTON; }
		if (std::holds_alternative<MOUSE>(_input))			{ return INPUT_VARIANT_ENUM_TYPE::MOUSE; }
		throw std::invalid_argument("InputUtils::GetInputType() - default case reached."); //This shouldn't happen unless I add a new type to the variant and forget to update this function
	}



	std::uint32_t InputUtils::GetGLFWKeyboardKey(const KEYBOARD _key)
	{
		switch (_key)
		{
		case KEYBOARD::Q: return GLFW_KEY_Q;
		case KEYBOARD::W: return GLFW_KEY_W;
		case KEYBOARD::E: return GLFW_KEY_E;
		case KEYBOARD::R: return GLFW_KEY_R;
		case KEYBOARD::T: return GLFW_KEY_T;
		case KEYBOARD::Y: return GLFW_KEY_Y;
		case KEYBOARD::U: return GLFW_KEY_U;
		case KEYBOARD::I: return GLFW_KEY_I;
		case KEYBOARD::O: return GLFW_KEY_O;
		case KEYBOARD::P: return GLFW_KEY_P;
		case KEYBOARD::A: return GLFW_KEY_A;
		case KEYBOARD::S: return GLFW_KEY_S;
		case KEYBOARD::D: return GLFW_KEY_D;
		case KEYBOARD::F: return GLFW_KEY_F;
		case KEYBOARD::G: return GLFW_KEY_G;
		case KEYBOARD::H: return GLFW_KEY_H;
		case KEYBOARD::J: return GLFW_KEY_J;
		case KEYBOARD::K: return GLFW_KEY_K;
		case KEYBOARD::L: return GLFW_KEY_L;
		case KEYBOARD::Z: return GLFW_KEY_Z;
		case KEYBOARD::X: return GLFW_KEY_X;
		case KEYBOARD::C: return GLFW_KEY_C;
		case KEYBOARD::V: return GLFW_KEY_V;
		case KEYBOARD::B: return GLFW_KEY_B;
		case KEYBOARD::N: return GLFW_KEY_N;
		case KEYBOARD::M: return GLFW_KEY_M;
		case KEYBOARD::NUM_1: return GLFW_KEY_1;
		case KEYBOARD::NUM_2: return GLFW_KEY_2;
		case KEYBOARD::NUM_3: return GLFW_KEY_3;
		case KEYBOARD::NUM_4: return GLFW_KEY_4;
		case KEYBOARD::NUM_5: return GLFW_KEY_5;
		case KEYBOARD::NUM_6: return GLFW_KEY_6;
		case KEYBOARD::NUM_7: return GLFW_KEY_7;
		case KEYBOARD::NUM_8: return GLFW_KEY_8;
		case KEYBOARD::NUM_9: return GLFW_KEY_9;
		case KEYBOARD::NUM_0: return GLFW_KEY_0;
		case KEYBOARD::ESCAPE: return GLFW_KEY_ESCAPE;
		default:
		{
			throw std::invalid_argument("GetGLFWKeyboardKey() default case reached. _key = " + std::to_string(std::to_underlying(_key)));
		}
		}
	}



	std::uint32_t InputUtils::GetGLFWMouseButton(const MOUSE_BUTTON _button)
	{
		switch (_button)
		{
		case MOUSE_BUTTON::LEFT:	return GLFW_MOUSE_BUTTON_LEFT;
		case MOUSE_BUTTON::RIGHT:	return GLFW_MOUSE_BUTTON_RIGHT;
		default:
		{
			throw std::invalid_argument("GetGLFWMouseButton() default case reached. _button = " + std::to_string(std::to_underlying(_button)));
		}
		}
	}

}