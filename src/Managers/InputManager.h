#pragma once

#include <Graphics/Window.h>

#include <unordered_map>
#include <variant>


namespace NK
{

	enum class INPUT_ACTION
	{
		MOVE_FORWARDS,
		MOVE_BACKWARDS,
		MOVE_LEFT,
		MOVE_RIGHT,
		CAMERA_YAW,
		CAMERA_PITCH,
	};

	enum class KEY_INPUT_STATE
	{
		HELD,
		NOT_HELD,
		//Todo: look into adding PRESSED and RELEASED states (for first frame of input)
	};

	typedef double MOUSE_INPUT_STATE;
	using INPUT_STATE = std::variant<KEY_INPUT_STATE, MOUSE_INPUT_STATE>;

	
	//Static class for storing input action states
	class InputManager
	{
	public:
		//Updates the actionStates map with the current input state on _window
		static void Update(Window* _window);
		
		[[nodiscard]] inline static INPUT_STATE GetInputState(const INPUT_ACTION _action) { return m_actionStates.at(_action); }
		[[nodiscard]] inline static INPUT_STATE GetInputStateLastUpdate(const INPUT_ACTION _action) { return m_actionStatesLastUpdate.at(_action); }


	private:
		inline static std::unordered_map<INPUT_ACTION, INPUT_STATE> m_actionStates
		{
	        { INPUT_ACTION::MOVE_FORWARDS,	KEY_INPUT_STATE::NOT_HELD },
			{ INPUT_ACTION::MOVE_BACKWARDS,	KEY_INPUT_STATE::NOT_HELD },
			{ INPUT_ACTION::MOVE_LEFT,		KEY_INPUT_STATE::NOT_HELD },
			{ INPUT_ACTION::MOVE_RIGHT,		KEY_INPUT_STATE::NOT_HELD },
			{ INPUT_ACTION::CAMERA_YAW,		0.0 },
			{ INPUT_ACTION::CAMERA_PITCH,	0.0 },
		};
		inline static std::unordered_map<INPUT_ACTION, INPUT_STATE> m_actionStatesLastUpdate{ m_actionStates };

		inline static bool m_firstUpdate{ true };
	};
	
}