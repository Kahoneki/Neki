#pragma once

#include "BaseComponent.h"

#include <Core/Utils/Serialisation/Serialisation.h>
#include <Core/Utils/Serialisation/TypeRegistry.h>
#include <Types/NekiTypes.h>

#include <SFML/Network.hpp>


namespace NK
{
	
	struct CNetworkEvent final
	{
	public:
		template<typename EventPacket>
		CNetworkEvent(const EventPacket& _packet)
		{
			eventPacket << std::to_underlying(PACKET_CODE::EVENT);
			eventPacket << TypeRegistry::GetConstant(typeid(EventPacket));

			std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
			{
				cereal::BinaryOutputArchive archive(ss);
				archive(inputComponents);
			}
			eventPacket.append(ss.str().c_str(), ss.str().size());
		}
		
		SERIALISE_MEMBER_FUNC(eventPacket)


		sf::Packet eventPacket;
	};

}
