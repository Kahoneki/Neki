#pragma once

#include "Entity.h"

#include <stdexcept>
#include <unordered_map>
#include <vector>


namespace NK
{

	struct IComponentPool
	{
		virtual ~IComponentPool() = default;
		virtual void RemoveEntity(Entity _entity) = 0;
	};

	
	template<typename Component>
	struct ComponentPool final : public IComponentPool
	{
		virtual inline void RemoveEntity(const Entity _entity) override
		{
			if (!entityToIndex.contains(_entity))
			{
				throw std::invalid_argument("ComponentPool::RemoveEntity() - provided _entity is not in component pool.");
			}


			//Swap and pop for fast removal - O(1) :]
			const std::size_t removedEntityIndex{ entityToIndex.at(_entity) };
			const Entity lastEntity{ indexToEntity.back() };

			//Move last element's data to removed element's spot
			components[removedEntityIndex] = std::move(components.back());
			indexToEntity[removedEntityIndex] = lastEntity;

			//Last entity's data is now at removedEntityIndex, need to update the map
			entityToIndex[lastEntity] = removedEntityIndex;

			//Puttin' the pop in swap and pop
			components.pop_back();
			indexToEntity.pop_back();
			entityToIndex.erase(_entity);
		}
		
		
		std::vector<Component> components; //All components of this component type

		std::unordered_map<Entity, std::size_t> entityToIndex; //Mapping from entity to index into components vector - i.e. components[entityToIndex[_entity]] is the component of this type for _entity
		std::vector<Entity> indexToEntity; //Reverse mapping of `entityToIndex` map
	};
	
}