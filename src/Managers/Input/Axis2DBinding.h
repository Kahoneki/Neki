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

		
		//Axis2DBinding(const INPUT_VARIANT _input)
		INPUT_VARIANT input;
		INPUT_VARIANT_ENUM_TYPE type;

		//Axis2DBinding(const std::pair<Axis1DBinding,Axis1DBinding>&)
		bool doubleAxes; //True if Axis2DBinding(const std::pair<Axis1DBinding,Axis1DBinding>&) is called
		std::pair<Axis1DBinding,Axis1DBinding> axis1DBindings;
	};
	
}