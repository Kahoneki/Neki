#pragma once

#include "Entity.h"
#include "Registry.h"


namespace NK
{

	template<typename... Components>
	class ComponentView final
	{
	public:
		explicit ComponentView(Registry* _reg)
		: m_reg(_reg)
		{
			//Find smallest pool to iterate over
			std::size_t minSize{ SIZE_MAX };

			//Fold expression to find the smallest pool, this is so sick....
			([&]
			{
				const ComponentPool<Components>* componentPool{ m_reg->GetPool<Components>() };
				if (componentPool->components.size() < minSize)
				{
					minSize = componentPool->components.size();
					m_iteratingPoolEntities = &(componentPool->indexToEntity);
				}
			}(), ...);
		}


		//---------------------------------//
		//--------ITERATOR SUBCLASS--------//
		//---------------------------------//

		class iterator
		{
		public:
			explicit iterator(Registry* _reg, const std::vector<Entity>* _entities, const std::size_t _index)
			: m_reg(_reg), m_entities(_entities), m_index(_index)
			{
				FindNextValidEntity();
			}


			//Dereference operator - returns a tuple of Components for the current entity
			[[nodiscard]] inline auto operator*() const
			{
				Entity entity{ (*m_entities)[m_index] };
				return std::tie(m_reg->GetComponent<Components>(entity)...);
			}


			//Increment operator - move on to next entity with Components
			inline iterator& operator++()
			{
				++m_index;
				FindNextValidEntity();
				return *this; //For chaining
			}


			[[nodiscard]] inline bool operator==(const iterator& _other) const { return (m_entities == _other.m_entities) && (m_index == _other.m_index); }
			[[nodiscard]] inline bool operator!=(const iterator& _other) const { return (m_entities != _other.m_entities) || (m_index != _other.m_index); }


		private:
			//Updates index to point to next entity in m_entities that has all Components
			void FindNextValidEntity()
			{
				while (m_index < m_entities->size())
				{
					Entity entity{ (*m_entities)[m_index] };
					if ((m_reg->HasComponent<Components>(entity) && ...))
					{
						//Found a valid entity, stop
						return;
					}
					//Entity not valid, increment index
					++m_index;
				}
			}
			
			
			Registry* m_reg;
			const std::vector<Entity>* m_entities;
			std::size_t m_index; //Current index into m_entities
		};
		
		//----------------------------------------//
		//--------END OF ITERATOR SUBCLASS--------//
		//----------------------------------------//


		iterator begin() { return iterator(m_reg, m_iteratingPoolEntities, 0); }
		iterator end() { return iterator(m_reg, m_iteratingPoolEntities, m_iteratingPoolEntities->size()); }
		

	private:
		Registry* m_reg;

		//Iterate over entities in the smallest component pool for efficiency
		const std::vector<Entity>* m_iteratingPoolEntities;
	};

}