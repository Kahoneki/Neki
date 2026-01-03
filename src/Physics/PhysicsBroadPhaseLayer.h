#pragma once

#include <cstdint>


namespace NK
{
	class PhysicsBroadPhaseLayer final
	{
	public:
		explicit PhysicsBroadPhaseLayer(const std::uint8_t _id) : m_broadPhaseLayer(_id) {}
		
		[[nodiscard]] inline std::uint8_t GetValue() const { return m_broadPhaseLayer; }
        
		SERIALISE_MEMBER_FUNC(m_broadPhaseLayer)
		
		
	private:
		std::uint8_t m_broadPhaseLayer;
	};
}