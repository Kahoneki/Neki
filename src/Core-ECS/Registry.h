#pragma once

#include "ComponentView.h"

#include <Components/BaseComponent.h>

#include <bitset>
#include <cstdint>
#include <typeindex>
#include <unordered_map>


namespace NK
{

	static constexpr std::size_t MAX_COMPONENT_TYPES{ 512u };

	class Registry final
	{
	public:
		template<typename... Component>
		[[nodiscard]] inline ComponentView View()
		{

		}


	private:
		template<typename Component>
		[[nodiscard]] inline std::uint32_t GetComponentIndex()
		{
			//Increments on first call, then returns id thereafter
			static std::uint32_t componentID{ m_nextComponentIndex++ };
			return componentID;
		}
		static std::uint32_t m_nextComponentIndex;

		
		//Map from component ids to all components in registry of that type
		std::unordered_map<std::type_index, std::vector<BaseComponent*>> m_components;
	};

}