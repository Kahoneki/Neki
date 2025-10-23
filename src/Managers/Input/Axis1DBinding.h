#pragma once

#include "ButtonBinding.h"

#include <Core/Utils/InputUtils.h>
#include <Types/NekiTypes.h>

#include <stdexcept>


namespace NK
{

	struct Axis1DBinding final
	{
		explicit Axis1DBinding() = default;
		

		explicit Axis1DBinding(const INPUT_VARIANT _input)
		: input(_input), type(InputUtils::GetInputType(_input)), digital(false)/*, doubleAxes(false)*/
		{
			if (type == INPUT_VARIANT_ENUM_TYPE::MOUSE)
			{
				throw std::invalid_argument("Axis1DBinding::Axis1DBinding() - input of type MOUSE cannot be bound to a button binding - did you mean to make an Axis2DBinding?");
			}
		}


		//Create a digital Axis1DBinding with two ButtonBindings, _digitalValues lets you set what values _input1 and _input1 being pressed will correspond to
		//For example, you might decide you want to the A and D keys to an Axis1DBinding for horizontal movement with A mapping to -1 and D mapping to 1
		//The above example can be accomplished with Axis1DBinding({aBinding,dBinding}, {-1,1})
		explicit Axis1DBinding(const std::pair<ButtonBinding, ButtonBinding>& _bindings, const std::pair<float, float>& _digitalValues)
		: type(INPUT_VARIANT_ENUM_TYPE::NONE), digital(true), buttonBindings(_bindings), digitalValues(_digitalValues)/*, doubleAxes(false)*/ {}


		inline bool operator==(const Axis1DBinding& _other) const noexcept
		{
			//Check that they hold the same input type, if not, then they're obviously not equal
			if (input.index() != _other.input.index())
			{
				return false;
			}

			//Same type, visit and compare the values
			return std::visit([](const auto& _a, const auto& _b) -> bool
			{
				if constexpr (std::is_same_v<decltype(_a), decltype(_b)>) { return _a == _b; }
				return false; //Shouldn't ever be reached
			}, input, _other.input);
		}

		
		//Axis1DBinding(const INPUT_VARIANT _input)
		INPUT_VARIANT input;
		INPUT_VARIANT_ENUM_TYPE type;

		//Axis1DBinding(const std::pair<ButtonBinding,ButtonBinding>&)
		bool digital; //True if Axis1DBinding(const std::pair<ButtonBinding,ButtonBinding>&) is called
		std::pair<ButtonBinding, ButtonBinding> buttonBindings;
		std::pair<float, float> digitalValues; //Stores the mapped range of buttonBindings in the axis
	};

}



template<>
struct std::hash<NK::Axis1DBinding>
{
	inline std::size_t operator()(const NK::Axis1DBinding& _binding) const noexcept
	{
		//Start by hashing digital vs non-digital
		std::size_t result{ std::hash<bool>{}(_binding.digital) };

		if (_binding.digital)
		{
			//Combine hashes of digital members
			const std::size_t h1{ std::hash<NK::ButtonBinding>{}(_binding.buttonBindings.first) };
			const std::size_t h2{ std::hash<NK::ButtonBinding>{}(_binding.buttonBindings.second) };
			const std::size_t h3{ std::hash<float>{}(_binding.digitalValues.first) };
			const std::size_t h4{ std::hash<float>{}(_binding.digitalValues.second) };
			result ^= (h1 << 1);
			result ^= (h2 << 2);
			result ^= (h3 << 3);
			result ^= (h4 << 4);
		}
		else
		{
			//Just hash the native input variant
			result ^= (std::hash<NK::INPUT_VARIANT>{}(_binding.input) << 1);
		}
		
		return result;
	}
};