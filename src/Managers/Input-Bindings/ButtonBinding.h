#pragma once

#include <Core/Utils/InputUtils.h>
#include <Types/NekiTypes.h>

#include <stdexcept>


namespace NK
{

	struct ButtonBinding final
	{
		explicit ButtonBinding() = default;
		explicit ButtonBinding(const INPUT_VARIANT _input) : input(_input), type(InputUtils::GetInputType(_input))
		{
			if (type == INPUT_VARIANT_ENUM_TYPE::MOUSE)
			{
				throw std::invalid_argument("ButtonBinding::ButtonBinding() - input of type MOUSE cannot be bound to a button binding - did you mean to make an Axis2DBinding?");
			}
		}


		inline bool operator==(const ButtonBinding& _other) const noexcept
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

		
		INPUT_VARIANT input;
		INPUT_VARIANT_ENUM_TYPE type;
	};
	
}


template<>
struct std::hash<NK::ButtonBinding>
{
	inline std::size_t operator()(const NK::ButtonBinding& _binding) const noexcept
	{
		return std::hash<NK::INPUT_VARIANT>{}(_binding.input);
	}
};