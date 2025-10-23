#include "Axis1DInput.h"

#include <Managers/InputManager.h>


namespace NK
{

	Axis1DState Axis1DInput::GetState() const
	{

		if (binding.digital)
		{
			const ButtonState bs1{ buttonInputs.first.GetState() };
			const ButtonState bs2{ buttonInputs.second.GetState() };

			const float value{ (bs1.held * binding.digitalValues.first) + (bs2.held * binding.digitalValues.second) };
			return Axis1DState(value);
		}

//		if (binding.doubleAxes)
//		{
//			//https://cplusplus.com/forum/general/113069/
//			
//			const Axis1DState s1{ axis1DInputs.first.GetState() };
//			const Axis1DState s2{ axis1DInputs.second.GetState() };
//			
//			const float value1{ (s1.value - binding.doubleAxesFromRange1.first) / (binding.doubleAxesFromRange1.second - binding.doubleAxesFromRange1.first) * (binding.doubleAxesToRange1.second - binding.doubleAxesToRange1.first) + binding.doubleAxesToRange1.first };
//			const float value2{ (s2.value - binding.doubleAxesFromRange2.first) / (binding.doubleAxesFromRange2.second - binding.doubleAxesFromRange2.first) * (binding.doubleAxesToRange2.second - binding.doubleAxesToRange2.first) + binding.doubleAxesToRange2.first };
//
//			return Axis1DState(value1 + value2);
//		}

		//Native Axis1DBinding - none currently supported
		Axis1DState state{};
		switch (binding.type)
		{
		default:
		{
			throw std::runtime_error("Default case reached for Axis1DInput::GetState() - this indicates an internal error, please make a GitHub issue on the topic");
		}
		}

		return state;
	}
	
}