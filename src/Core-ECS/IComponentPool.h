#pragma once

#include "Entity.h"

#include <cereal/archives/binary.hpp>
#include <stdexcept>
#include <unordered_map>
#include <vector>


namespace NK
{

	class Registry;
	
	
	struct IComponentPool
	{
		virtual ~IComponentPool() = default;
		virtual void RemoveEntity(Entity _entity) = 0;
		//Returns the component for a given entity as a CImGuiInspectorRenderable* if possible
		virtual class CImGuiInspectorRenderable* GetAsImGuiInspectorRenderableComponent(Entity _entity) = 0;
		virtual constexpr bool IsImGuiInspectorRenderableType() const = 0;
		virtual std::string GetImGuiInspectorRenderableName() const = 0;
		virtual void AddDefaultToEntity(Registry& _reg, Entity _entity) = 0;
		virtual void CopyComponentToEntity(Registry& _reg, Entity _srcEntity, Entity _dstEntity) = 0;
		
		virtual void Serialise(cereal::BinaryOutputArchive& _archive) = 0;
		virtual void Deserialise(cereal::BinaryInputArchive& _archive) = 0;
		virtual const std::vector<Entity>& GetEntities() const = 0;
	};
	
}