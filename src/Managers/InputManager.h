#pragma once

#include "Input-Bindings/Axis2DBinding.h"

#include <Core/Utils/Serialisation/TypeRegistry.h>
#include <Graphics/Window.h>
#include <Types/NekiTypes.h>

#include <stdexcept>
#include <unordered_map>


namespace NK
{

	typedef std::variant<ButtonBinding, Axis1DBinding, Axis2DBinding> INPUT_BINDING_VARIANT;
	
	
	class InputManager final
	{
		friend class InputLayer;
		
		
	public:
		static inline void SetWindow(const Window* const _window) { m_window = _window; }
		

		template<typename ActionType>
		[[nodiscard]] inline static ButtonState GetButtonState(const ActionType _action)
		{
			ValidateActionTypeUtil("GetButtonState()", _action, INPUT_BINDING_TYPE::BUTTON);
			const ActionTypeMapKey key{ std::type_index(typeid(ActionType)), std::to_underlying(_action) };
			return GetButtonState(key);
		}


		template<typename ActionType>
		[[nodiscard]] inline static Axis1DState GetAxis1DState(const ActionType _action)
		{
			ValidateActionTypeUtil("GetAxis1DState()", _action, INPUT_BINDING_TYPE::AXIS_1D);
			const ActionTypeMapKey key{ typeid(ActionType).name(), std::to_underlying(_action) };
			return GetAxis1DState(key);
		}

		
		template<typename ActionType>
		[[nodiscard]] inline static Axis2DState GetAxis2DState(const ActionType _action)
		{
			ValidateActionTypeUtil("GetAxis2DState()", _action, INPUT_BINDING_TYPE::AXIS_2D);
			const ActionTypeMapKey key{ std::type_index(typeid(ActionType)), std::to_underlying(_action) };
			return GetAxis2DState(key);
		}
		

		template<typename ActionType>
		inline static void BindActionToInput(const ActionType _action, const INPUT_BINDING_VARIANT& _input)
		{
			const ActionTypeMapKey key{ TypeRegistry::GetConstant(std::type_index(typeid(ActionType))), std::to_underlying(_action) };

			if (std::holds_alternative<ButtonBinding>(_input))		{ m_actionToInputTypeMap[key] = INPUT_BINDING_TYPE::BUTTON; }
			else if (std::holds_alternative<Axis1DBinding>(_input))	{ m_actionToInputTypeMap[key] = INPUT_BINDING_TYPE::AXIS_1D; }
			else if (std::holds_alternative<Axis2DBinding>(_input))	{ m_actionToInputTypeMap[key] = INPUT_BINDING_TYPE::AXIS_2D; }
			else { throw std::invalid_argument("InputManager::BindActionToInput() - _input is not in expected list of INPUT_BINDING_VARIANT options. This indicates an internal error, please make a GitHub issue on the topic"); }

			m_actionToInputMap[key] = _input;
		}


		template<typename ActionType>
		inline static INPUT_BINDING_TYPE GetActionInputType(const ActionType _action)
		{
			const ActionTypeMapKey key{ TypeRegistry::GetConstant(std::type_index(typeid(ActionType))), std::to_underlying(_action) };
			return (m_actionToInputTypeMap.contains(key) ? m_actionToInputTypeMap[key] : INPUT_BINDING_TYPE::UNBOUND);
		}
		
		
		static void Update();
		

		[[nodiscard]] static bool GetKeyPressed(const KEYBOARD _key);
		[[nodiscard]] static bool GetKeyReleased(const KEYBOARD _key);
		[[nodiscard]] static bool GetMouseButtonPressed(const MOUSE_BUTTON _button);
		[[nodiscard]] static bool GetMouseButtonReleased(const MOUSE_BUTTON _button);
		[[nodiscard]] static glm::vec2 GetMouseDiff();
		[[nodiscard]] static glm::vec2 GetMousePosition();
		static INPUT_BINDING_TYPE GetActionInputType(const ActionTypeMapKey& _key);
		

	private:
		template<typename ActionType>
		inline static void ValidateActionTypeUtil(const std::string& _func, const ActionType _action, const INPUT_BINDING_TYPE _inputType)
		{
			const ActionTypeMapKey key{ TypeRegistry::GetConstant(std::type_index(typeid(ActionType))), std::to_underlying(_action) };
			ValidateActionTypeUtil(_func, key, _inputType);
		}
		
		
		//Used by InputLayer
		[[nodiscard]] static ButtonState GetButtonState(const ActionTypeMapKey& _key);
		[[nodiscard]] static Axis1DState GetAxis1DState(const ActionTypeMapKey& _key);
		[[nodiscard]] static Axis2DState GetAxis2DState(const ActionTypeMapKey& _key);
		
		static void ValidateActionTypeUtil(const std::string& _func, const ActionTypeMapKey& _key, const INPUT_BINDING_TYPE _inputType);
		static ButtonState EvaluateButtonBinding(const ButtonBinding& _binding);
		static Axis1DState EvaluateAxis1DBinding(const Axis1DBinding& _binding);
		static Axis2DState EvaluateAxis2DBinding(const Axis2DBinding& _binding);
		

		inline static const Window* m_window{ nullptr };

		inline static std::unordered_map<ActionTypeMapKey, INPUT_BINDING_TYPE> m_actionToInputTypeMap; //Map from action to the type of input it's bound to
		inline static std::unordered_map<ActionTypeMapKey, INPUT_BINDING_VARIANT> m_actionToInputMap; //Map from action to the input it's bound to

		inline static glm::vec2 m_mousePosLastFrame{ 0, 0 };
		inline static glm::vec2 m_mousePosThisFrame{ 0, 0 };
		inline static bool m_firstUpdate{ true }; //Used to avoid a big mouse delta on first frame
	};

}