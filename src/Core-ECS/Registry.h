#pragma once

#include "ComponentPool.h"

#include <Core/Memory/Allocation.h>
#include <Core/Memory/FreeListAllocator.h>
#include <Managers/EventManager.h>

#include <algorithm>
#include <memory>
#include <typeindex>
#include <unordered_map>


namespace NK
{

	//Forward declaration
	template<typename... Components>
	class ComponentView;


	
	class Registry final
	{
		template<typename... Components>
		friend class ComponentView;
		
		
	public:
		explicit Registry(std::size_t _maxEntities)
		: m_entityAllocator(NK_NEW(FreeListAllocator, _maxEntities)) {}


		~Registry()
		{
			while (!m_entityComponents.empty())
			{
				Destroy(m_entityComponents.begin()->first);
			}
		}


		//Get a ComponentView for the provided Components that can be iterated through
		//Each element in the ComponentView iterator is a tuple of Components for an entity that has those components
		//Looping through the entirety of the ComponentView's iterator will go through all entities with the provided Components combination 
		template<typename... Components>
		[[nodiscard]] inline ComponentView<Components...> View();


		//Add a new entity to the registry
		[[nodiscard]] inline Entity Create()
		{
			const Entity newEntity{ m_entityAllocator->Allocate() };
			if (newEntity == FreeListAllocator::INVALID_INDEX)
			{
				throw std::runtime_error("Registry::Create() - max entities reached!");
			}
			m_entityComponents[newEntity] = {};
			return newEntity;
		}


		//Remove _entity from the registry
		inline void Destroy(const Entity _entity)
		{
			if (!m_entityComponents.contains(_entity))
			{
				throw std::invalid_argument("Registry::Destroy() - provided _entity (" + std::to_string(_entity) + ") is not in registry.");
			}
			
			EventManager::Trigger(EntityDestroyEvent(this, _entity));
			
			for (std::type_index type : m_entityComponents[_entity])
			{
				m_componentPools[type]->RemoveEntity(_entity);
			}
			m_entityComponents.erase(_entity);
			m_entityAllocator->Free(_entity);
		}

		
		//Add Component component to _entity with ComponentArgs
		//E.g.: reg.AddComponent<HealthComponent>(player, 100.0f, 50.0f);
		template<typename Component, typename... ComponentArgs>
		inline Component& AddComponent(Entity _entity, ComponentArgs&&... _componentArgs)
		{
			if (!m_entityComponents.contains(_entity))
			{
				throw std::invalid_argument("Registry::AddComponent() - provided _entity (" + std::to_string(_entity) + ") is not in registry.");
			}

			if (HasComponent<Component>(_entity))
			{
				throw std::invalid_argument("Registry::AddComponent() - provided _entity (" + std::to_string(_entity) + ") already has the provided Component.");
			}
			
			//Add new component to pool
			ComponentPool<Component>* pool{ GetPool<Component>() };
			pool->components.emplace_back(std::forward<ComponentArgs>(_componentArgs)...);

			//Update pool's lookup fields
			const std::size_t newIndex{ pool->components.size() - 1 };
			pool->entityToIndex[_entity] = newIndex;
			pool->indexToEntity.push_back(_entity);

			//Update entityComponents map
			m_entityComponents[_entity].push_back(std::type_index(typeid(Component)));

			return pool->components.back();
		}

		
		//Remove Component component from _entity
		template<typename Component>
		inline void RemoveComponent(Entity _entity)
		{
			if (!m_entityComponents.contains(_entity))
			{
				throw std::invalid_argument("Registry::RemoveComponent() - provided _entity (" + std::to_string(_entity) + ") is not in registry.");
			}

			const std::vector<std::type_index>::iterator entityComponentsIt{ std::ranges::find(m_entityComponents[_entity], std::type_index(typeid(Component))) };
			if (entityComponentsIt == m_entityComponents[_entity].end())
			{
				throw std::invalid_argument("Registry::RemoveComponent() - provided _entity (" + std::to_string(_entity) + ") does not contain the provided component.");
			}
			
			ComponentPool<Component>* pool{ GetPool<Component>() };
			pool->RemoveEntity(_entity);
			m_entityComponents[_entity].erase(entityComponentsIt);
		}

		
		//Returns true if _entity has Component component
		template<typename Component>
		[[nodiscard]] inline bool HasComponent(const Entity _entity) const
		{
			if (!m_entityComponents.contains(_entity))
			{
				throw std::invalid_argument("Registry::HasComponent() - provided _entity (" + std::to_string(_entity) + ") is not in registry.");
			}

			const ComponentPool<Component>* componentPool{ GetPool<Component>() };
			if (componentPool == nullptr)
			{
				return false;
			}
			return GetPool<Component>()->entityToIndex.contains(_entity);
		}
		
		
		//Returns true if _entity has component with type index _index
		[[nodiscard]] inline bool HasComponent(const Entity _entity, const std::type_index _index) const
		{
			if (!m_entityComponents.contains(_entity))
			{
				throw std::invalid_argument("Registry::HasComponent() - provided _entity (" + std::to_string(_entity) + ") is not in registry.");
			}

			return (std::ranges::find(m_entityComponents.at(_entity), _index) != m_entityComponents.at(_entity).end());
		}

		
		//Returns const-reference to Component component on _entity
		template<typename Component>
		[[nodiscard]] inline const Component& GetComponent(const Entity _entity) const
		{
			if (!m_entityComponents.contains(_entity))
			{
				throw std::invalid_argument("Registry::RemoveComponent() - provided _entity (" + std::to_string(_entity) + ") is not in registry.");
			}
			
			const ComponentPool<Component>* pool{ GetPool<Component>() };
			if (!pool || !pool->entityToIndex.contains(_entity))
			{
				throw std::invalid_argument("Registry::GetComponent() - provided _entity (" + std::to_string(_entity) + ") does not have the provided component.");
			}
			return pool->components[pool->entityToIndex.at(_entity)];
		}

		
		//Returns reference to Component component on _entity
		template<typename Component>
		[[nodiscard]] inline Component& GetComponent(const Entity _entity)
		{
			if (!m_entityComponents.contains(_entity))
			{
				throw std::invalid_argument("Registry::RemoveComponent() - provided _entity (" + std::to_string(_entity) + ") is not in registry.");
			}
			
			ComponentPool<Component>* pool{ GetPool<Component>() };
			if (!pool->entityToIndex.contains(_entity))
			{
				throw std::invalid_argument("Registry::GetComponent() - provided _entity (" + std::to_string(_entity) + ") does not have the provided component.");
			}
			return pool->components[pool->entityToIndex.at(_entity)];
		}


		//Returns entity that contains a given component
		template<typename Component>
		[[nodiscard]] Entity GetEntity(const Component& _component) const
		{
			const ComponentPool<Component>* pool{ GetPool<Component>() };
			if (!pool)
			{
				throw std::invalid_argument("Registry::GetEntity() - no pool exists for the provided _component type");
			}

			//Calculate index of component within its pool's vector
			const Component* poolComponentsBegin{ pool->components.data() };
			const Component* componentPtr{ &_component };
			if ((componentPtr < poolComponentsBegin) || (componentPtr >= poolComponentsBegin + pool->components.size()))
			{
				throw std::invalid_argument("Registry::GetEntity() - provided component does not belong to this registry");
			}
			return pool->indexToEntity[componentPtr - poolComponentsBegin];
		}
	
		
		[[nodiscard]] inline std::vector<std::type_index> GetEntityComponents(const Entity _entity) const { return m_entityComponents.at(_entity); }
		[[nodiscard]] inline IComponentPool* GetPool(const std::type_index _index) const { return m_componentPools.at(_index).get(); }
		[[nodiscard]] inline bool EntityInRegistry(const Entity _entity) const { return m_entityComponents.contains(_entity); }
		
		
	private:
		template<typename Component>
		[[nodiscard]] inline const ComponentPool<Component>* GetPool() const
		{
			const std::type_index componentIndex{ std::type_index(typeid(Component)) };
			if (!m_componentPools.contains(componentIndex))
			{
				return nullptr;
			}
			return static_cast<const ComponentPool<Component>*>(m_componentPools.at(componentIndex).get());
		}
		
		
		template<typename Component>
		[[nodiscard]] inline ComponentPool<Component>* GetPool()
		{
			const std::type_index componentIndex{ std::type_index(typeid(Component)) };
			if (!m_componentPools.contains(componentIndex))
			{
				m_componentPools[componentIndex] = std::make_unique<ComponentPool<Component>>();
			}
			return static_cast<ComponentPool<Component>*>(m_componentPools.at(componentIndex).get());
		}

		
		//Map from component id to component pool containing all components in registry of that type
		std::unordered_map<std::type_index, std::unique_ptr<IComponentPool>> m_componentPools;

		UniquePtr<FreeListAllocator> m_entityAllocator;

		//Map from entity to all component ids the entity has
		std::unordered_map<Entity, std::vector<std::type_index>> m_entityComponents;
	};

}


#include "Registry.inl"