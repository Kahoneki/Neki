#pragma once
#include <Core-ECS/Registry.h>


namespace NK
{

	class Scene
	{
		friend class Engine;
		
		
	public:
		Scene(const std::size_t _maxEntities) : m_reg(_maxEntities) {}
		virtual ~Scene() = default;


	protected:
		Registry m_reg;
	};
	
}