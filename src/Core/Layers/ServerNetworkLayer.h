#pragma once

#include "ILayer.h"

#include <Core-ECS/Registry.h>
#include <SFML/Network.hpp>
#include <Types/NekiTypes.h>


namespace NK
{

	struct ServerNetworkLayerDesc
	{
		SERVER_TYPE type;
		std::uint32_t maxClients;
		double portClaimTimeout{ 999999 }; //Time in seconds the server is allowed to try and claim the port for before timing out
		std::uint32_t maxTCPPacketsPerClientPerTick{ 128u }; //If a client sends more TCP packets than this in a single tick, they will be kicked from the server
		std::uint32_t maxUDPPacketsPerClientPerTick{ 512u }; //If a client sends more TCP packets than this in a single tick, they will be kicked from the server
	};
	
	
	class ServerNetworkLayer final : public ILayer
	{
	public:
		explicit ServerNetworkLayer(ILogger& _logger, const ServerNetworkLayerDesc& _desc);
		virtual ~ServerNetworkLayer() override;

		virtual void Update(Registry& _reg) override;
		NETWORK_LAYER_ERROR_CODE Host(const unsigned short _port);
		
		
	private:
		//Init sub-functions
		[[nodiscard]] NETWORK_LAYER_ERROR_CODE CheckForIncomingConnectionRequests();
		[[nodiscard]] NETWORK_LAYER_ERROR_CODE CheckForIncomingTCPData(Registry& _reg);
		[[nodiscard]] NETWORK_LAYER_ERROR_CODE CheckForIncomingUDPData(Registry& _reg);
		std::unordered_map<ClientIndex, sf::TcpSocket>::iterator DisconnectClient(const ClientIndex _index); //Returns iterator to next valid iterator position in m_connectedClientTCPSockets
		
		
		ServerNetworkLayerDesc m_desc;
		SERVER_STATE m_state;
		std::optional<sf::IpAddress> m_address;
		unsigned short m_port;
		sf::TcpListener m_tcpListener;
		sf::TcpSocket m_tcpSocket;
		sf::UdpSocket m_udpSocket;
		std::unordered_map<std::string, ClientIndex> m_connectedClients; //For fast lookup (client ip -> client index)
		std::unordered_map<ClientIndex, sf::TcpSocket> m_connectedClientTCPSockets;
		std::unordered_map<ClientIndex, unsigned short> m_connectedClientUDPPorts;
		UniquePtr<FreeListAllocator> m_clientIndexAllocator;
		ClientIndex m_nextClientIndex;
	};

}