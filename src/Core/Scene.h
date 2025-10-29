#pragma once
#include <Core-ECS/Registry.h>


namespace NK
{

	class Scene
	{
	public:
		explicit Scene(const std::size_t _maxEntities) : m_reg(_maxEntities) {}
		virtual ~Scene() = default;

		virtual void Update() = 0;
		
		Registry m_reg;
	};
	
}