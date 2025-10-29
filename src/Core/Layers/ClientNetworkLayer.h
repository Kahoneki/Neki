#pragma once

#include "ILayer.h"

#include <Core-ECS/Registry.h>
#include <SFML/Network.hpp>
#include <Types/NekiTypes.h>


namespace NK
{

	struct ClientNetworkLayerDesc
	{
		explicit ClientNetworkLayerDesc(const double _serverConnectTimeout, const double _serverClientIndexPacketTimeout)
		: serverConnectTimeout(_serverConnectTimeout), serverClientIndexPacketTimeout(_serverClientIndexPacketTimeout) {}

		ClientNetworkLayerDesc() {}

		double serverConnectTimeout{ 5.0 }; //Time in seconds the client is allowed to spend trying to connect to the server before timing out
		double serverClientIndexPacketTimeout{ 5.0 }; //Time in seconds the client is allowed to spend waiting to receive their client index packet from the server
	};
	
	
	class ClientNetworkLayer final : public ILayer
	{
	public:
		explicit ClientNetworkLayer(Registry& _reg, const ClientNetworkLayerDesc& _desc);
		virtual ~ClientNetworkLayer() override;

		virtual void Update() override;
		NETWORK_LAYER_ERROR_CODE Connect(const char* _ip, const unsigned short _port);
		NETWORK_LAYER_ERROR_CODE Disconnect();
		
		
	private:
		void PreAppUpdate();
		void PostAppUpdate();
		
		ClientNetworkLayerDesc m_desc;
		CLIENT_STATE m_state;
		sf::TcpSocket m_tcpSocket;
		sf::UdpSocket m_udpSocket;
		std::optional<sf::IpAddress> m_serverAddress;
		unsigned short m_serverPort;
		ClientIndex m_index;
	};

}