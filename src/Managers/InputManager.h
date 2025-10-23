#pragma once

#include "Input/Axis1DInput.h"
#include "Input/Axis2DInput.h"
#include "Input/BaseInput.h"
#include "Input/ButtonInput.h"

#include <Graphics/Window.h>
#include <Types/NekiTypes.h>

#include <stdexcept>
#include <unordered_map>


#define ACTION_TYPE_TEMPLATE_FUNC template<typename ActionType>

namespace NK
{
	
	class InputManager final
	{
		friend class InputLayer;
		
		
	public:
		static inline void SetWindow(const Window* const _window) { m_window = _window; }
		
		
		ACTION_TYPE_TEMPLATE_FUNC [[nodiscard]] inline static ButtonState GetButtonState(const ActionType _action)
		{
			const ActionTypeMapKey key{ std::type_index(typeid(ActionType)), std::to_underlying(_action) };
			ValidateActionTypeUtil("GetButtonState()", _action, INPUT_TYPE::BUTTON);
			return dynamic_cast<const ButtonInput* const>(m_actionToInputMap[key])->GetState();
		}

		
		ACTION_TYPE_TEMPLATE_FUNC [[nodiscard]] inline static Axis1DState GetAxis1DState(const ActionType _action)
		{
			const ActionTypeMapKey key{ std::type_index(typeid(ActionType)), std::to_underlying(_action) };
			ValidateActionTypeUtil("GetAxis1DState()", _action, INPUT_TYPE::AXIS_1D);
			return dynamic_cast<const Axis1DInput* const>(m_actionToInputMap[key])->GetState();
		}


		ACTION_TYPE_TEMPLATE_FUNC [[nodiscard]] inline static Axis2DState GetAxis2DState(const ActionType _action)
		{
			const ActionTypeMapKey key{ std::type_index(typeid(ActionType)), std::to_underlying(_action) };
			ValidateActionTypeUtil("GetAxis2DState()", _action, INPUT_TYPE::AXIS_2D);
			return dynamic_cast<const Axis2DInput* const>(m_actionToInputMap[key])->GetState();
		}
		

		ACTION_TYPE_TEMPLATE_FUNC inline static void BindActionToInput(const ActionType _action, const BaseInput* const _input, const INPUT_TYPE _inputType)
		{
			const ActionTypeMapKey key{ std::type_index(typeid(ActionType)), std::to_underlying(_action) };
			m_actionToInputTypeMap[key] = _inputType;
			m_actionToInputMap[key] = _input;
		}


		ACTION_TYPE_TEMPLATE_FUNC inline static INPUT_TYPE GetActionInputType(const ActionType _action)
		{
			const ActionTypeMapKey key{ std::type_index(typeid(ActionType)), std::to_underlying(_action) };
			return (m_actionToInputTypeMap.contains(key) ? m_actionToInputTypeMap[key] : INPUT_TYPE::UNBOUND);
		}
		

		//To be called every frame if NK::MOUSE is being used
		static void UpdateMouse();
		

		[[nodiscard]] static bool GetKeyPressed(const KEYBOARD _key);
		[[nodiscard]] static bool GetKeyReleased(const KEYBOARD _key);
		[[nodiscard]] static bool GetMouseButtonPressed(const MOUSE_BUTTON _button);
		[[nodiscard]] static bool GetMouseButtonReleased(const MOUSE_BUTTON _button);
		[[nodiscard]] static glm::vec2 GetMouseDiff();
		[[nodiscard]] static glm::vec2 GetMousePosition();
		

	private:
		ACTION_TYPE_TEMPLATE_FUNC inline static void ValidateActionTypeUtil(const std::string& _func, const ActionType _action, const INPUT_TYPE _inputType)
		{
			const ActionTypeMapKey key{ std::type_index(typeid(ActionType)), std::to_underlying(_action) };
			ValidateActionTypeUtil(_func, key, _inputType);
		}


		
		//Used by InputLayer
		static INPUT_TYPE GetActionInputType(const ActionTypeMapKey& _key);
		[[nodiscard]] static ButtonState GetButtonState(const ActionTypeMapKey& _key);
		[[nodiscard]] static Axis1DState GetAxis1DState(const ActionTypeMapKey& _key);
		[[nodiscard]] static Axis2DState GetAxis2DState(const ActionTypeMapKey& _key);
		static void ValidateActionTypeUtil(const std::string& _func, const ActionTypeMapKey& _key, const INPUT_TYPE _inputType);


		inline static const Window* m_window{ nullptr };

		inline static std::unordered_map<ActionTypeMapKey, INPUT_TYPE> m_actionToInputTypeMap; //Map from action to the type of input it's bound to
		inline static std::unordered_map<ActionTypeMapKey, const BaseInput*> m_actionToInputMap; //Map from action to the input it's bound to

		inline static glm::vec2 m_mousePosLastFrame{ 0, 0 };
		inline static glm::vec2 m_mousePosThisFrame{ 0, 0 };
		inline static bool m_firstUpdate{ true }; //Used to avoid a big mouse delta on first frame
	};

}