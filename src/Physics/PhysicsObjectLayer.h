#pragma once

#include "PhysicsBroadPhaseLayer.h"


namespace NK
{
	class PhysicsObjectLayer final
	{
	public:
		explicit PhysicsObjectLayer(const std::uint16_t _id, const PhysicsBroadPhaseLayer _broadPhaseLayer) : m_objectLayer(_id), m_broadPhaseLayer(_broadPhaseLayer) {}
		
		[[nodiscard]] inline std::uint16_t GetValue() const { return m_objectLayer; }
		[[nodiscard]] inline PhysicsBroadPhaseLayer GetBroadPhaseLayer() const { return m_broadPhaseLayer; }
		
		
		inline bool operator==(const PhysicsObjectLayer& _other) const 
		{
			return (m_objectLayer == _other.m_objectLayer);
		}
        
		
	private:
		std::uint16_t m_objectLayer;
		PhysicsBroadPhaseLayer m_broadPhaseLayer;
	};
}