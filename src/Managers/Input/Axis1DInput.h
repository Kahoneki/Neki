#pragma once

#include "Axis1DBinding.h"
#include "BaseInput.h"
#include "ButtonInput.h"

#include <Graphics/Window.h>


namespace NK
{

	struct Axis1DInput final : public BaseInput
	{
	public:
		explicit Axis1DInput() = default;
		explicit Axis1DInput(const Axis1DBinding& _binding)
		: binding(_binding),
		buttonInputs({ ButtonInput(binding.buttonBindings.first), ButtonInput(binding.buttonBindings.second) }) /*,
		axis1DInputs({ Axis1DInput(binding.axis1DBindings.first), Axis1DInput(binding.axis1DBindings.second) }) */ {}
		
		[[nodiscard]] Axis1DState GetState() const;


	private:
		Axis1DBinding binding;
		std::pair<ButtonInput, ButtonInput> buttonInputs;
//		const std::pair<Axis1DInput, Axis1DInput> axis1DInputs;
	};
	
}