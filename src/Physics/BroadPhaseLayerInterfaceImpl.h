#pragma once

#include <Types/NekiTypes.h>
#ifdef max
	#undef max
#endif


namespace NK
{
	
	class BroadPhaseLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
	{
	public:
		explicit BroadPhaseLayerInterfaceImpl(const PhysicsLayerDesc& _desc)
		{
			m_numBroadPhaseLayers = 0;
			for (const auto& layer : _desc.objectLayers)
			{
				m_numBroadPhaseLayers = std::max(m_numBroadPhaseLayers, layer.GetBroadPhaseLayer().GetValue());
			}
			++m_numBroadPhaseLayers;
			
			for (const PhysicsObjectLayer& objectLayer : _desc.objectLayers)
			{
				m_objectToBroadPhase[objectLayer.GetValue()] = JPH::BroadPhaseLayer(objectLayer.GetBroadPhaseLayer().GetValue());
			}
		}
		
		inline virtual JPH::uint GetNumBroadPhaseLayers() const override { return m_numBroadPhaseLayers; }
		inline virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(const JPH::ObjectLayer _inLayer) const override { return m_objectToBroadPhase.at(_inLayer); }
		inline virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override { return "Unnamed"; } //todo: can i be bothered to expose this to the user?
		
		
	private:
		std::uint8_t m_numBroadPhaseLayers;
		//The broad phase layer that any given object layer is in
		std::unordered_map<JPH::ObjectLayer, JPH::BroadPhaseLayer> m_objectToBroadPhase;
	};
	
}