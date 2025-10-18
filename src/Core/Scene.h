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

		virtual void Update() = 0;


	protected:
		Registry m_reg;
	};
	
}