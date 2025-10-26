#pragma once

#include "ILayer.h"

#include <Core-ECS/Registry.h>
#include <SFML/Network.hpp>
#include <Types/NekiTypes.h>


namespace NK
{

	struct ServerNetworkLayerDesc
	{
		explicit ServerNetworkLayerDesc(const SERVER_TYPE _type, const std::uint32_t _maxClients, const double _portClaimTimeout, const std::uint32_t _maxTCPPacketsPerClientTick, const std::uint32_t _maxUDPPacketsPerClientTick)
		: type(_type), maxClients(_maxClients), portClaimTimeout(_portClaimTimeout), maxTCPPacketsPerClientPerTick(_maxTCPPacketsPerClientTick), maxUDPPacketsPerClientPerTick(_maxUDPPacketsPerClientTick) {}

		ServerNetworkLayerDesc() {}

		SERVER_TYPE type{ SERVER_TYPE::LAN };
		std::uint32_t maxClients{ 0 };
		double portClaimTimeout{ 999999 }; //Time in seconds the server is allowed to try and claim the port for before timing out
		std::uint32_t maxTCPPacketsPerClientPerTick{ 128u }; //If a client sends more TCP packets than this in a single tick, they will be kicked from the server
		std::uint32_t maxUDPPacketsPerClientPerTick{ 512u }; //If a client sends more TCP packets than this in a single tick, they will be kicked from the server
	};
	
	
	class ServerNetworkLayer final : public ILayer
	{
	public:
		explicit ServerNetworkLayer(Registry& _reg, const ServerNetworkLayerDesc& _desc);
		virtual ~ServerNetworkLayer() override;

		virtual void Update() override;
		NETWORK_LAYER_ERROR_CODE Host(const unsigned short _port);
		
		
	private:
		//Init sub-functions
		[[nodiscard]] NETWORK_LAYER_ERROR_CODE CheckForIncomingConnectionRequests();
		[[nodiscard]] NETWORK_LAYER_ERROR_CODE CheckForIncomingTCPData();
		[[nodiscard]] NETWORK_LAYER_ERROR_CODE CheckForIncomingUDPData();
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