#pragma once

#include <Core/Memory/Allocation.h>
#include <Core-ECS/IComponentPool.h>

#include <cstdint>
#include <string>
#include <typeindex>
#include <unordered_map>


namespace NK
{

	template<typename Component>
	struct ComponentPool;
	
	
	//std::type_index can't be serialised because it's a raw pointer and the underlying index is apparently not guaranteed constant between program executions
	//typeid(T).name() is also not serialisable because it's implementation defined (wtf? https://en.cppreference.com/w/cpp/types/type_info/name)
	//so this class basically just holds a static map between actual constant values that can persist between implementations and between and `std::type_index`s
	class TypeRegistry
	{
	public:
		template<typename T>
		inline static void Register(const std::string& _typeName)
		{
			const std::uint32_t hashVal{ StringToUintHash(_typeName) };
			m_registryValueToTypeIndex.emplace(hashVal, std::type_index(typeid(T)));
			m_typeIndexToRegistryValue.emplace(std::type_index(typeid(T)), hashVal);
			m_poolFactories[hashVal] = []{ return UniquePtr<IComponentPool>(NK_NEW(ComponentPool<T>)); };
		}


		[[nodiscard]] inline static std::uint32_t GetConstant(const std::type_index _typeIndex) { return m_typeIndexToRegistryValue.at(_typeIndex); }
		[[nodiscard]] inline static std::type_index GetTypeIndex(const std::uint32_t _constant) { return m_registryValueToTypeIndex.at(_constant); }
		
		[[nodiscard]] inline static UniquePtr<IComponentPool> CreatePoolFromHash(const std::uint32_t _hash)
		{
			if (!m_poolFactories.contains(_hash)) { throw std::invalid_argument("TypeRegistry::CreatePoolFromHash() - Unknown component hash. Did you forget to TypeRegistry::Register() it?"); }
			return m_poolFactories[_hash]();
		}
		
		
	private:
		constexpr inline static std::uint32_t StringToUintHash(const std::string& _str)
		{
			//https://stackoverflow.com/a/51276700
			std::uint32_t hash{ 0x811c9dc5 };

			for(int i = 0; i < _str.size(); ++i) {
				const std::uint8_t value{ static_cast<std::uint8_t>(_str[i]) };
				hash = hash ^ value;
				hash *= 0x1000193;
			}

			return hash;
		}
		
		
		inline static std::unordered_map<std::uint32_t, std::type_index> m_registryValueToTypeIndex{};
		inline static std::unordered_map<std::type_index, std::uint32_t> m_typeIndexToRegistryValue{}; //Reverse mapping of m_registryValueToTypeIndex
		static inline std::unordered_map<std::uint32_t, std::function<UniquePtr<IComponentPool>()>> m_poolFactories{};
	};
	
}