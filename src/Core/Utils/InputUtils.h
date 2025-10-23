#pragma once

#include <Types/NekiTypes.h>


namespace NK
{
	
	class InputUtils
	{
	public:
		[[nodiscard]] static INPUT_VARIANT_ENUM_TYPE GetInputType(const INPUT_VARIANT _input);
		[[nodiscard]] static std::uint32_t GetGLFWKeyboardKey(const KEYBOARD _key);
		[[nodiscard]] static std::uint32_t GetGLFWMouseButton(const MOUSE_BUTTON _button);
	};
	
}