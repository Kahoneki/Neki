#pragma once

#include "Axis1DInput.h"
#include "Axis2DBinding.h"
#include "BaseInput.h"
#include "ButtonInput.h"

#include <Graphics/Window.h>


namespace NK
{

	struct Axis2DInput final : public BaseInput
	{
	public:
		explicit Axis2DInput() = default;
		explicit Axis2DInput(const Axis2DBinding& _binding)
		: binding(_binding),
		axis1DInputs({ Axis1DInput(binding.axis1DBindings.first), Axis1DInput(binding.axis1DBindings.second) }) {}
		
		[[nodiscard]] Axis2DState GetState() const;


	private:
		Axis2DBinding binding;
		std::pair<Axis1DInput, Axis1DInput> axis1DInputs;
	};
	
}