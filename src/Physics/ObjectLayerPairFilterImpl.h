#pragma once

#include <Types/NekiTypes.h>

#include <Jolt/Physics/PhysicsSystem.h>


namespace NK
{
	
	
	class ObjectLayerPairFilterImpl final : public JPH::ObjectLayerPairFilter
	{
	public:
		explicit ObjectLayerPairFilterImpl(const PhysicsLayerDesc& _desc)
		{
			for (std::unordered_map<PhysicsObjectLayer, std::vector<PhysicsObjectLayer>>::const_iterator it{ _desc.objectLayerCollisionPartners.begin() }; it != _desc.objectLayerCollisionPartners.end(); ++it)
			{
				m_objectLayerCollisionPartners[it->first.GetValue()].resize(it->second.size());
				for (std::size_t i{ 0 }; i < it->second.size(); ++i)
				{
					m_objectLayerCollisionPartners[it->first.GetValue()][i] = it->second[i].GetValue();
				}
			}
		}
		
		
		inline virtual bool ShouldCollide(const JPH::ObjectLayer _inLayer1, const JPH::ObjectLayer _inLayer2) const override
		{
			//todo: this is really inefficient, use a 2d bitmask lut instead
			return (std::ranges::find(m_objectLayerCollisionPartners.at(_inLayer1), _inLayer2) != m_objectLayerCollisionPartners.at(_inLayer1).end());
		}
		
		
	private:
		std::unordered_map<JPH::ObjectLayer, std::vector<JPH::ObjectLayer>> m_objectLayerCollisionPartners;
	};
	
}