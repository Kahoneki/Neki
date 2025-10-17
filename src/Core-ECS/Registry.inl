#pragma once //Unnecessary for .inl?

#include "ComponentView.h"

namespace NK
{

	//Define in here to get around circular dependency with ComponentView - i still dont fully understand how this works, but shoutout stack overflow
	template<typename... Components>
	[[nodiscard]] inline ComponentView<Components...> Registry::View()
	{
		return ComponentView<Components...>(this);
	}
	
}