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



//		//Create an Axis1DBinding with two Axis1DBindings, this lets you map two Axis1DBindings to a single continuous Axis1DBinding
//		//For example, if you have a car with a velocity value in the range -1 to 1, you may naturally decide you want to express this as a single Axis1DBinding
//		//You can then choose to bind the negative range of the velocity to the left trigger and the positive range to the right trigger, creating one continuous Axis1DBinding based on the two values
//		//To accomplish the above example, assuming the input range of the triggers are both -1 to 1, this would be accomplished with Axis1DBinding({leftTrigBinding,rightTrigBinding}, {-1,1}, {-1,0}, {-1,1}, {0,1})
//		//
//		//_fromRange1 is the native range of _binding1, _toRange1 is the range you want _binding1 to be mapped to
//		//_fromRange2 is the native range of _binding2, _toRange2 is the range you want _binding2 to be mapped to
//		explicit Axis1DBinding(const std::pair<const Axis1DBinding* const, const Axis1DBinding* const>& _bindings, const std::pair<float, float>& _fromRange1, const std::pair<float, float>& _toRange1, const std::pair<float, float>& _fromRange2, const std::pair<float, float>& _toRange2)
//		: type(INPUT_VARIANT_ENUM_TYPE::NONE), digital(false), doubleAxes(true), axis1DBindings(_bindings),
//		  doubleAxesFromRange1(_fromRange1), doubleAxesToRange1(_toRange1), doubleAxesFromRange2(_fromRange2), doubleAxesToRange2(_toRange2) {}



		//Axis1DBinding(const INPUT_VARIANT _input)
		INPUT_VARIANT input;
		INPUT_VARIANT_ENUM_TYPE type;

		//Axis1DBinding(const std::pair<ButtonBinding,ButtonBinding>&)
		bool digital; //True if Axis1DBinding(const std::pair<ButtonBinding,ButtonBinding>&) is called
		std::pair<ButtonBinding, ButtonBinding> buttonBindings;
		std::pair<float, float> digitalValues; //Stores the mapped range of buttonBindings in the axis

//		//Axis1DBinding(const std::pair<const Axis1DBinding* const, const Axis1DBinding* const>&)
//		bool doubleAxes; //True if Axis1DBinding(const std::pair<Axis1DBinding,Axis1DBinding>&) is called
//		const std::pair<const Axis1DBinding* const, const Axis1DBinding* const> axis1DBindings;
//		const std::pair<float, float> doubleAxesFromRange1; //Stores the native range of axis1DBindings.first
//		const std::pair<float, float> doubleAxesToRange1; //Stores the mapped range of axis1DBindings.first in the axis
//		const std::pair<float, float> doubleAxesFromRange2; //Stores the native range of axis1DBindings.second
//		const std::pair<float, float> doubleAxesToRange2; //Stores the mapped range of axis1DBindings.second in the axis
	};

}