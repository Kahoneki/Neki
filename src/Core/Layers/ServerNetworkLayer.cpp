#include "ServerNetworkLayer.h"

#include <Core/Utils/Timer.h>


namespace NK
{

	ServerNetworkLayer::ServerNetworkLayer(const ServerNetworkLayerDesc& _desc)
	: m_desc(_desc), m_state(SERVER_STATE::NOT_HOSTING), m_clientIndexAllocator(NK_NEW(FreeListAllocator, m_desc.maxClients))
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::SERVER_NETWORK_LAYER, "Initialising Server Network Layer\n");
		m_logger.Unindent();
	}



	ServerNetworkLayer::~ServerNetworkLayer()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::SERVER_NETWORK_LAYER, "Shutting Down Server Network Layer\n");
		m_logger.Unindent();
	}



	void ServerNetworkLayer::Update(Registry& _reg)
	{
		m_logger.Indent();
		
		
		if (m_nextClientIndex != FreeListAllocator::INVALID_INDEX)
		{
			const NETWORK_LAYER_ERROR_CODE err{ CheckForIncomingConnectionRequests() };
			if (err != NETWORK_LAYER_ERROR_CODE::SUCCESS)
			{
				m_logger.Unindent();
				return;
			}
		}

		NETWORK_LAYER_ERROR_CODE err{ CheckForIncomingTCPData(_reg) };
		if (err != NETWORK_LAYER_ERROR_CODE::SUCCESS)
		{
			m_logger.Unindent();
			return;
		}

		err = CheckForIncomingUDPData(_reg);
		if (err != NETWORK_LAYER_ERROR_CODE::SUCCESS)
		{
			m_logger.Unindent();
			return;
		}


		m_logger.Unindent();
	}



	NETWORK_LAYER_ERROR_CODE ServerNetworkLayer::Host(const unsigned short _port)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::SERVER_NETWORK_LAYER, "Host() Called For Port " + std::to_string(_port) + "\n");
		
		if (m_state != SERVER_STATE::NOT_HOSTING)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::SERVER_NETWORK_LAYER, "Host() called on a ServerNetworkLayer whose m_state != SERVER_STATE::NOT_HOSTING - m_state = " + std::to_string(std::to_underlying(m_state)));
			return NETWORK_LAYER_ERROR_CODE::SERVER__HOST_CALLED_ON_HOSTING_SERVER;
		}

		m_address = (m_desc.type == SERVER_TYPE::LAN ? sf::IpAddress::getLocalAddress() : sf::IpAddress::getPublicAddress());
		m_port = _port;

		
		//Timer for tracking timeout
		Timer timer{ m_desc.portClaimTimeout };

		
		//Assign TCP listener to port
		//Repeatedly try to claim the port until successful or timeout
		m_tcpListener.setBlocking(true);
		while (m_tcpListener.listen(m_port) != sf::Socket::Status::Done)
		{
			if (timer.IsComplete())
			{
				//Timeout reached
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::SERVER_NETWORK_LAYER, "Timeout reached while attempting to bind TCP listener to port\n");
				m_logger.Unindent();
				return NETWORK_LAYER_ERROR_CODE::SERVER__TCP_LISTENER_PORT_CLAIM_TIME_OUT;
			}
			sf::sleep(sf::milliseconds(20));
			timer.Update();
		}
		m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::SERVER_NETWORK_LAYER, "TCP listener successfully bound to port\n");
		m_tcpListener.setBlocking(false);

		
		//Assign UDP socket to port
		//Repeatedly try to claim the port until successful or timeout
		timer.Reset(m_desc.portClaimTimeout);
		m_udpSocket.setBlocking(true);
		while (m_udpSocket.bind(m_port) != sf::Socket::Status::Done)
		{
			if (timer.IsComplete())
			{
				//Timeout reached
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::SERVER_NETWORK_LAYER, "Timeout reached while attempting to bind UDP socket to port\n");
				m_logger.Unindent();
				return NETWORK_LAYER_ERROR_CODE::SERVER__UDP_SOCKET_PORT_CLAIM_TIME_OUT;
			}
			sf::sleep(sf::milliseconds(20));
			timer.Update();
		}
		m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::SERVER_NETWORK_LAYER, "UDP socket successfully bound to port\n");
		m_udpSocket.setBlocking(false);

		m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::SERVER_NETWORK_LAYER, std::string(m_desc.type == SERVER_TYPE::LAN ? "LAN" : "WAN") + " server set up on " + m_address.value().toString() + ":" + std::to_string(m_port) + "\n");

		
		m_nextClientIndex = m_clientIndexAllocator->Allocate();
		m_logger.Unindent();
		return NETWORK_LAYER_ERROR_CODE::SUCCESS;
	}



	NETWORK_LAYER_ERROR_CODE ServerNetworkLayer::CheckForIncomingConnectionRequests()
	{
		//Create a new socket at a free client index and check if there's a connection request

		//Create a temporary socket to accept the new connection
		sf::TcpSocket socket;
		socket.setBlocking(false);
		if (m_tcpListener.accept(socket) == sf::Socket::Status::Done)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::SERVER_NETWORK_LAYER, "Client (address: " + socket.getRemoteAddress()->toString() + ":" + std::to_string(socket.getRemotePort()) + ") connected to the server.\n");

			//Connection was successful, add to map
			m_connectedClientTCPSockets[m_nextClientIndex] = std::move(socket);
			m_connectedClients[m_connectedClientTCPSockets[m_nextClientIndex].getRemoteAddress()->toString()] = m_nextClientIndex;
			
			//Send client index back to client
			sf::Packet outgoingPacket;
			outgoingPacket << m_nextClientIndex;
			if (m_connectedClientTCPSockets[m_nextClientIndex].send(outgoingPacket) != sf::Socket::Status::Done)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::SERVER_NETWORK_LAYER, "Failed to send client index back to client\n");
				m_connectedClientTCPSockets.erase(m_nextClientIndex);
				return NETWORK_LAYER_ERROR_CODE::SERVER__FAILED_TO_SEND_CLIENT_INDEX_PACKET;
			}

			m_nextClientIndex = m_clientIndexAllocator->Allocate();
		}
		
		
		return NETWORK_LAYER_ERROR_CODE::SUCCESS;
	}



	NETWORK_LAYER_ERROR_CODE ServerNetworkLayer::CheckForIncomingTCPData(Registry& _reg)
	{
		//In the event of an error, store error code in err rather than returning immediately
		//^Returning immediately could lead to, for example, a client whom was able to repeatedly flood the server with error packets being able to stop data for subsequent client being received
		NETWORK_LAYER_ERROR_CODE err{ NETWORK_LAYER_ERROR_CODE::SUCCESS };

		
		//Split up the gathering of packets which is very fast from the processing of packets which might be (relatively) much slower
		//This stops the server getting stuck in an infinite loop if packets are being sent faster than they can be processed

		
		//Gather packets
		//Loop through all connected TCP sockets to see if data is being received
		std::unordered_map<ClientIndex, std::vector<sf::Packet>> clientPackets;
		for (std::unordered_map<ClientIndex, sf::TcpSocket>::iterator it{ m_connectedClientTCPSockets.begin() }; it != m_connectedClientTCPSockets.end(); ++it)
		{			
			const ClientIndex& index{ it->first };
			
			sf::Packet incomingData;
			while (it->second.receive(incomingData) == sf::Socket::Status::Done)
			{
				clientPackets[index].push_back(incomingData);

				//Ensure client hasn't sent more TCP packets this tick than is allowed
				if (clientPackets[index].size() > m_desc.maxTCPPacketsPerClientPerTick)
				{
					m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::SERVER_NETWORK_LAYER, "Client (index = " + std::to_string(index) + ", address = " + it->second.getRemoteAddress()->toString() + ":" + std::to_string(it->second.getRemotePort()) + ") attempted to send " + std::to_string(clientPackets[index].size()) + " TCP packets this tick - this exceeds the limit set in m_desc.maxTCPPacketsPerClientPerTick (" + std::to_string(m_desc.maxTCPPacketsPerClientPerTick) + ") - disconnecting them\n");
					it = DisconnectClient(index);
					clientPackets.erase(index);
					break;
				}
			}
		}

		
		//Process packets
		for (std::unordered_map<ClientIndex, std::vector<sf::Packet>>::iterator it{ clientPackets.begin() }; it != clientPackets.end(); ++it)
		{
			//So that we can update the iterator
			bool clientDisconnect{ false };
			
			for (sf::Packet& packet : it->second)
			{
				std::underlying_type_t<PACKET_CODE> underlyingPacketCode;
				packet >> underlyingPacketCode;
				switch (const PACKET_CODE code{ underlyingPacketCode })
				{
				case PACKET_CODE::DISCONNECT:
				{
					m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::SERVER_NETWORK_LAYER, "Packet received: DISCONNECT\n");
					DisconnectClient(it->first);
					clientDisconnect = true;
					break;
				}
				default:
				{
					m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::SERVER_NETWORK_LAYER, "Client sent invalid packet code - code = " + std::to_string(underlyingPacketCode) + " - disconnecting them\n");
					DisconnectClient(it->first);
					clientDisconnect = true;
					break;
				}
				}

				//If the client has been disconnected (or an attempt has been made to disconnect the client), don't process any of their other packets for this tick
				if (clientDisconnect)
				{
					break;
				}
			}
		}


		return err;
	}



	NETWORK_LAYER_ERROR_CODE ServerNetworkLayer::CheckForIncomingUDPData(Registry& _reg)
	{
		//In the event of an error, store error code in err rather than returning immediately
		//^Returning immediately could lead to, for example, a client whom was able to repeatedly flood the server with error packets being able to stop data for subsequent client being received
		NETWORK_LAYER_ERROR_CODE err{ NETWORK_LAYER_ERROR_CODE::SUCCESS };

		
		//Split up the gathering of packets which is very fast from the processing of packets which might be (relatively) much slower
		//This stops the server getting stuck in an infinite loop if packets are being sent faster than they can be processed

		
		//Gather packets
		std::unordered_map<ClientIndex, std::vector<sf::Packet>> clientPackets;
		sf::Packet incomingData;
		std::optional<sf::IpAddress> incomingClientIP;
		unsigned short incomingClientPort;
		m_udpSocket.setBlocking(false);
		while (m_udpSocket.receive(incomingData, incomingClientIP, incomingClientPort) == sf::Socket::Status::Done)
		{
			//Make sure client is connected through TCP
			if (!m_connectedClients.contains(incomingClientIP->toString().c_str()))
			{
				m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::SERVER_NETWORK_LAYER, "Client at address " + incomingClientIP->toString() + ":" + std::to_string(incomingClientPort) + " attempted to send UDP packet without being connected through TCP\n");

				//There's not really much we can do in this case since the client isn't connected to us
				//Due to the nature of UDP, this could simply happen because the user sent a UDP packet then a TCP disconnect packet, and the UDP packet wasn't received until after the TCP disconnect packet
				//Because of this, it doesn't necessarily indicate any malicious intent from the client, so just ignore it and continue to the next packet
				continue;
			}
			
			const ClientIndex& index{ m_connectedClients[incomingClientIP->toString().c_str()] };
			clientPackets[index].push_back(incomingData);

			//Ensure client hasn't sent more UDP packets this tick than is allowed
			if (clientPackets[index].size() > m_desc.maxUDPPacketsPerClientPerTick)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::SERVER_NETWORK_LAYER, "Client (index = " + std::to_string(index) + ", address = " + incomingClientIP->toString() + ":" + std::to_string(incomingClientPort) + ") attempted to send " + std::to_string(clientPackets[index].size()) + " UDP packets this tick - this exceeds the limit set in m_desc.maxUDPPacketsPerClientPerTick (" + std::to_string(m_desc.maxUDPPacketsPerClientPerTick) + ") - disconnecting them\n");
				DisconnectClient(index);
				clientPackets.erase(index);
			}

			incomingData.clear();
		}

		
		//Process packets
		for (std::unordered_map<ClientIndex, std::vector<sf::Packet>>::iterator it{ clientPackets.begin() }; it != clientPackets.end(); ++it)
		{
			//So that we don't process anymore of a client's packets after a disconnect packet
			bool clientDisconnect{ false };

			for (sf::Packet& packet : it->second)
			{
				std::underlying_type_t<PACKET_CODE> underlyingPacketCode;
				packet >> underlyingPacketCode;
				const PACKET_CODE code{ underlyingPacketCode };

				//Ensure that if client isn't connected through UDP that they aren't try to send anything other than PACKET_CODE::CLIENT_INDEX_FOR_UDP_PORT
				if (!m_connectedClientUDPPorts.contains(it->first))
				{
					if (code != PACKET_CODE::UDP_PORT)
					{
						m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::SERVER_NETWORK_LAYER, "Client (index = " + std::to_string(it->first) + ", address = " + incomingClientIP->toString() + ":" + std::to_string(incomingClientPort) + ") attempted to send a UDP packet that was not PACKET_CODE::CLIENT_INDEX_FOR_UDP_PORT without being connected through UDP (code = " + std::to_string(underlyingPacketCode) + ") - disconnecting them\n");
						DisconnectClient(it->first);
						clientDisconnect = true;
						break;
					}
				}

				switch (code)
				{
				case PACKET_CODE::UDP_PORT:
				{
					m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::SERVER_NETWORK_LAYER, "Packet received: UDP_PORT\n");
					m_connectedClientUDPPorts[it->first] = incomingClientPort;
					break;
				}
				default:
				{
					m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::SERVER_NETWORK_LAYER, "Client sent invalid packet code - code = " + std::to_string(underlyingPacketCode) + " - disconnecting them\n");
					DisconnectClient(it->first);
					clientDisconnect = true;
					break;
				}
				}
			}
			
			//If the client has been disconnected (or an attempt has been made to disconnect the client), don't process any of their other packets for this tick
			if (clientDisconnect)
			{
				it = clientPackets.erase(it);
			}
		}


		return err;
	}



	std::unordered_map<ClientIndex, sf::TcpSocket>::iterator ServerNetworkLayer::DisconnectClient(const ClientIndex _index)
	{
		m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::SERVER_NETWORK_LAYER, "Client (index: " + std::to_string(_index) + ") disconnected from the server\n");
		if (m_connectedClientUDPPorts.contains(_index)) { m_connectedClientUDPPorts.erase(_index); }
		if (m_connectedClients.contains(m_connectedClientTCPSockets[_index].getRemoteAddress()->toString())) { m_connectedClients.erase(m_connectedClientTCPSockets[_index].getRemoteAddress()->toString()); }
		m_clientIndexAllocator->Free(_index);
		m_nextClientIndex = m_clientIndexAllocator->Allocate();
		return m_connectedClientTCPSockets.erase(m_connectedClientTCPSockets.find(_index));
	}

}