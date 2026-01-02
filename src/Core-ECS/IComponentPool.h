#pragma once

#include "Entity.h"

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
		virtual bool IsImGuiInspectorRenderableType() const = 0;
		virtual std::string GetImGuiInspectorRenderableName() const = 0;
		virtual void AddDefaultToEntity(Registry& _reg, Entity _entity) = 0;
	};
	
}