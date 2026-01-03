#pragma once

#include "IComponentPool.h"


namespace NK
{

	
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
		
		
		virtual inline CImGuiInspectorRenderable* GetAsImGuiInspectorRenderableComponent(const Entity _entity) override
		{
			if constexpr (std::is_base_of_v<CImGuiInspectorRenderable, Component>)
			{
				if (entityToIndex.contains(_entity))
				{
					return static_cast<CImGuiInspectorRenderable*>(&components[entityToIndex.at(_entity)]);
				}
			}
			return nullptr;
		}
		
		
		virtual constexpr inline bool IsImGuiInspectorRenderableType() const override
		{
			return std::is_base_of_v<CImGuiInspectorRenderable, Component>;
		}
		
		
		virtual inline std::string GetImGuiInspectorRenderableName() const override
		{
			if constexpr (IsImGuiInspectorRenderableType())
			{
				return Component::GetStaticName();
			}
			return "Unnamed";
		}
		
		
		virtual inline void Serialise(cereal::BinaryOutputArchive& _archive) override
		{
			_archive(components, entityToIndex, indexToEntity);
		}
		
		
		virtual inline void Deserialise(cereal::BinaryInputArchive& _archive) override
		{
			_archive(components, entityToIndex, indexToEntity);
		}
		
		
		virtual const std::vector<Entity>& GetEntities() const override { return indexToEntity; }
		
		
		virtual void AddDefaultToEntity(Registry& _reg, const Entity _entity) override;
		virtual void CopyComponentToEntity(Registry& _reg, const Entity _srcEntity, const Entity _dstEntity) override;
		
		
		std::vector<Component> components; //All components of this component type

		std::unordered_map<Entity, std::size_t> entityToIndex; //Mapping from entity to index into components vector - i.e. components[entityToIndex[_entity]] is the component of this type for _entity
		std::vector<Entity> indexToEntity; //Reverse mapping of `entityToIndex` map
	};
	
}
