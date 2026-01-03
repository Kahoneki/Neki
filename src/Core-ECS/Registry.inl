#pragma once //Unnecessary for .inl?

#include "ComponentView.h"

namespace NK
{

	//Define in here to get around circular dependency with ComponentView - i still dont fully understand how this works, but shoutout stack overflow
	template<typename... Components>
	[[nodiscard]] inline ComponentView<Components...> Registry::View()
	{
		return ComponentView<Components...>(this);
	}
	
	
	inline void Registry::Save(const std::string& _filepath)
	{
		for (auto&& [transform] : View<CTransform>())
		{
			transform.OnBeforeSerialise(*this);
		}
		
		std::ofstream os(_filepath, std::ios::binary);
		if (!os.is_open())
		{
			throw std::runtime_error("Registry::Save() - Failed to open filepath (" + _filepath +") for saving.");
		}
		cereal::BinaryOutputArchive archive(os);
		archive(*m_entityAllocator);
		
		archive(m_componentPools.size());
		for (std::unordered_map<std::type_index, UniquePtr<IComponentPool>>::iterator it{ m_componentPools.begin() }; it != m_componentPools.end(); ++it)
		{
			archive(TypeRegistry::GetConstant(it->first));
			it->second->Serialise(archive);
		}
	}
	
	
	inline void Registry::Load(const std::string& _filepath)
	{
		if (!std::filesystem::exists(_filepath))
		{
			throw std::runtime_error("Registry::Load() - Failed to open filepath (" + _filepath +") for loading.");
		}
		
		std::vector<Entity> entitiesToDestroy;
		entitiesToDestroy.reserve(m_entityComponents.size());
		for (const auto& [entity, components] : m_entityComponents)
		{
			entitiesToDestroy.push_back(entity);
		}
		for (const Entity entity : entitiesToDestroy)
		{
			Destroy(entity);
		}
		
		EventManager::Trigger(SceneLoadEvent());
		
		m_entityComponents.clear();
		m_componentPools.clear();
		
		std::ifstream is(_filepath, std::ios::binary);
		cereal::BinaryInputArchive archive(is);

		archive(*m_entityAllocator);

		std::size_t poolCount;
		archive(poolCount);
		for (std::size_t i{ 0 }; i < poolCount; ++i)
		{
			std::uint32_t typeHash;
			archive(typeHash);

			UniquePtr<IComponentPool> newPool{ TypeRegistry::CreatePoolFromHash(typeHash) };
			newPool->Deserialise(archive);

			std::type_index typeIdx = TypeRegistry::GetTypeIndex(typeHash);
			m_componentPools[typeIdx] = std::move(newPool);
		}

		for (auto& [typeIdx, pool] : m_componentPools)
		{
			const std::vector<Entity>& entities = pool->GetEntities();
			for (Entity e : entities)
			{
				m_entityComponents[e].push_back(typeIdx);
			}
		}
		
		for (auto&& [transform] : View<CTransform>())
		{
			if (transform.serialisedParentID != UINT32_MAX)
			{
				if (HasComponent<CTransform>(transform.serialisedParentID))
				{
					CTransform* parentPtr = &GetComponent<CTransform>(transform.serialisedParentID);
					transform.SetParent(*this, parentPtr);
				}
			}
			else
			{
				transform.SetParent(*this, nullptr);
			}
		}
    }
	
}