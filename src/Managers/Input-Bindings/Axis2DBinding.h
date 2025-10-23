#pragma once

#include "Axis1DBinding.h"

#include <Core/Utils/InputUtils.h>
#include <Types/NekiTypes.h>


namespace NK
{

	struct Axis2DBinding final
	{
		explicit Axis2DBinding() = default;
		
		explicit Axis2DBinding(const INPUT_VARIANT _input)
		: input(_input), type(InputUtils::GetInputType(_input)), doubleAxes(false) {}

		//Create an Axis2DBinding with two Axis1DBinding
		//For example, you might decide you want to express movement as an Axis2DInput, requiring an Axis2DBinding
		//On keyboard, it's natural to have this bound to WASD (4 digital inputs)
		//This can be accomplished by creating an Axis1DBinding with A and D for the horizontal binding and likewise an Axis1DBinding with W and S for the vertical binding
		//Then using this constructor like so: Axis2DBinding({horizontalBinding,verticalBinding})
		explicit Axis2DBinding(const std::pair<Axis1DBinding,Axis1DBinding>& _bindings)
		: type(INPUT_VARIANT_ENUM_TYPE::NONE), doubleAxes(true), axis1DBindings(_bindings) {}


		inline bool operator==(const Axis2DBinding& _other) const noexcept
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

		
		//Axis2DBinding(const INPUT_VARIANT _input)
		INPUT_VARIANT input;
		INPUT_VARIANT_ENUM_TYPE type;

		//Axis2DBinding(const std::pair<Axis1DBinding,Axis1DBinding>&)
		bool doubleAxes; //True if Axis2DBinding(const std::pair<Axis1DBinding,Axis1DBinding>&) is called
		std::pair<Axis1DBinding,Axis1DBinding> axis1DBindings;
	};
	
}



template<>
struct std::hash<NK::Axis2DBinding>
{
	inline std::size_t operator()(const NK::Axis2DBinding& _binding) const noexcept
	{
		//Start by hashing doubleAxes vs non-doubleAxes
		std::size_t result{ std::hash<bool>{}(_binding.doubleAxes) };

		if (_binding.doubleAxes)
		{
			//Combine hashes of Axis1DBinding members
			const std::size_t h1{ std::hash<NK::Axis1DBinding>{}(_binding.axis1DBindings.first) };
			const std::size_t h2{ std::hash<NK::Axis1DBinding>{}(_binding.axis1DBindings.second) };
			result ^= (h1 << 1);
			result ^= (h2 << 2);
		}
		else
		{
			//Just hash the native input variant
			result ^= (std::hash<NK::INPUT_VARIANT>{}(_binding.input) << 1);
		}
		
		return result;
	}
};