#include "Axis2DInput.h"

#include <Managers/InputManager.h>


namespace NK
{

	Axis2DState Axis2DInput::GetState() const
	{

		if (binding.doubleAxes)
		{
			const Axis1DState as1{ axis1DInputs.first.GetState() };
			const Axis1DState as2{ axis1DInputs.second.GetState() };

			return Axis2DState({ as1.value, as2.value });
		}


		//Native Axis2DBinding
		switch (binding.type)
		{
		case (INPUT_VARIANT_ENUM_TYPE::MOUSE):
		{
			const MOUSE mouseInputType{ std::get<MOUSE>(binding.input) };
			if (mouseInputType == MOUSE::POSITION)
			{
				return Axis2DState(InputManager::GetMousePosition());
			}
			if (mouseInputType == MOUSE::POSITION_DIFFERENCE)
			{
				return Axis2DState(InputManager::GetMouseDiff());
			}
			throw std::runtime_error("mouseInputType not recognised in Axis2DInput::GetState() - this indicates an internal error, please make a GitHub issue on the topic");
		}
		default:
		{
			throw std::runtime_error("Default case reached for Axis2DInput::GetState() - this indicates an internal error, please make a GitHub issue on the topic");
		}
		}
	}
	
}