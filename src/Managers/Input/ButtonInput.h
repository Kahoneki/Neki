#pragma once

#include "BaseInput.h"
#include "ButtonBinding.h"

#include <Graphics/Window.h>


namespace NK
{

	struct ButtonInput final : public BaseInput
	{
	public:
		explicit ButtonInput() = default;
		explicit ButtonInput(const ButtonBinding& _binding) : binding(_binding) {}
		[[nodiscard]] ButtonState GetState() const;


	private:
		ButtonBinding binding;
	};
	
}