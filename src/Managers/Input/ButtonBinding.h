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

		
		INPUT_VARIANT input;
		INPUT_VARIANT_ENUM_TYPE type;
	};
	
}