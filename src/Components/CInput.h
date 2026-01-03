#pragma once

#include <Core/Utils/Serialisation/TypeRegistry.h>
#include <Types/NekiTypes.h>

#include <stdexcept>
#include <unordered_map>


namespace NK
{
	
	struct CInput final
	{
	public:
		//Helper function to add an action state to the actionStates map
		template<typename ActionType>
		inline void AddActionToMap(ActionType _action)
		{
			const ActionTypeMapKey key{ TypeRegistry::GetConstant(std::type_index(typeid(ActionType))), std::to_underlying(_action) };
			actionStates[key] = {};
		}


		//Helper function to remove an action state from the actionStates map
		template<typename ActionType>
		inline void RemoveActionFromMap(ActionType _action)
		{
			const ActionTypeMapKey key{ std::type_index(typeid(ActionType)), std::to_underlying(_action) };
			const std::unordered_map<ActionTypeMapKey, INPUT_STATE_VARIANT>::iterator it{ actionStates.find(key) };
			if (it == actionStates.end()) { throw std::invalid_argument("CInput::RemoveActionFromMap() - invalid action provided - type: " + std::string(_action.name()) + ", element: " + std::to_string(_action)); }
			actionStates.erase(it);
		}

		
		//Helper function to safely get the state
		//Fun fact apparently this compiles despite seemingly creating multiple identical functions only separated by return type because templates will change the name of the function
		template<typename T, typename ActionType>
		T GetActionState(ActionType _action) const
		{
			const ActionTypeMapKey key{ TypeRegistry::GetConstant(std::type_index(typeid(ActionType))), std::to_underlying(_action) };
			const std::unordered_map<ActionTypeMapKey, INPUT_STATE_VARIANT>::const_iterator it{ actionStates.find(key) };
			if (it == actionStates.end()) { return T{}; }
			return std::get<T>(it->second);
		}
		
		
		SERIALISE_MEMBER_FUNC(actionStates)
		
		
		//Map from user's action-types enum (represented as a pair of both the enum class' type index and the std::uint32_t underlying type) to its current state
		std::unordered_map<ActionTypeMapKey, INPUT_STATE_VARIANT> actionStates;
	};
	
}
