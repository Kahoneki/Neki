#pragma once

#include "BaseComponent.h"

#include <typeindex>
#include <unordered_map>


namespace NK
{

	struct Archetype
	{
	public:
		std::unordered_map<std::type_index, std::vector<BaseComponent>*> m_components;
	};

}