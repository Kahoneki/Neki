#pragma once

#include <Core-ECS/Registry.h>
#include <SFML/Network.hpp>
#include <Types/NekiTypes.h>


namespace NK
{

	struct NetworkSystemDesc
	{
		ServerSettings server{};
		ClientSettings client{};
	};
	
	
	class NetworkSystem final
	{
	public:
		explicit NetworkSystem(ILogger& _logger, const NetworkSystemDesc& _desc);
		~NetworkSystem();

		NETWORK_SYSTEM_ERROR_CODE Update(Registry& _reg);
		NETWORK_SYSTEM_ERROR_CODE Host(const unsigned short _port);
		NETWORK_SYSTEM_ERROR_CODE Connect(const char* _ip, const unsigned short _port);
		NETWORK_SYSTEM_ERROR_CODE Disconnect();
		
		
	private:
		[[nodiscard]] NETWORK_SYSTEM_ERROR_CODE ServerUpdate(Registry& _reg);
		[[nodiscard]] NETWORK_SYSTEM_ERROR_CODE S_CheckForIncomingConnectionRequests();
		[[nodiscard]] NETWORK_SYSTEM_ERROR_CODE S_CheckForIncomingTCPData(Registry& _reg);
		[[nodiscard]] NETWORK_SYSTEM_ERROR_CODE S_CheckForIncomingUDPData(Registry& _reg);
		std::unordered_map<ClientIndex, sf::TcpSocket>::iterator S_DisconnectClient(const ClientIndex _index); //Returns iterator to next valid iterator position in ms_connectedClientTCPSockets
		
		[[nodiscard]] NETWORK_SYSTEM_ERROR_CODE ClientUpdate(Registry& _reg);
		
		
		//Dependency injections
		ILogger& m_logger;

		NETWORK_SYSTEM_TYPE m_type;

		//Server
		ServerSettings ms_settings;
		std::optional<sf::IpAddress> ms_address;
		unsigned short ms_port;
		sf::TcpListener ms_tcpListener;
		sf::TcpSocket ms_tcpSocket;
		sf::UdpSocket ms_udpSocket;
		std::unordered_map<const char*, ClientIndex> ms_connectedClients; //For fast lookup (client ip -> client index)
		std::unordered_map<ClientIndex, sf::TcpSocket> ms_connectedClientTCPSockets;
		std::unordered_map<ClientIndex, unsigned short> ms_connectedClientUDPPorts;
		UniquePtr<FreeListAllocator> ms_clientIndexAllocator;
		ClientIndex ms_nextClientIndex;

		//Client
		ClientSettings mc_settings;
		sf::TcpSocket mc_tcpSocket;
		sf::UdpSocket mc_udpSocket;
		std::optional<sf::IpAddress> mc_serverAddress;
		unsigned short mc_serverPort;
		ClientIndex mc_index;
		bool mc_connected;
	};

}