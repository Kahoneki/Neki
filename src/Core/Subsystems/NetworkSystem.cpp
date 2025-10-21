#include "NetworkSystem.h"

#include <Core/Utils/Timer.h>


namespace NK
{

	NetworkSystem::NetworkSystem(ILogger& _logger, const NetworkSystemDesc& _desc)
	: m_logger(_logger), m_type(NETWORK_SYSTEM_TYPE::NONE),
	ms_settings(_desc.server),  ms_clientIndexAllocator(NK_NEW(FreeListAllocator, ms_settings.maxClients)),
	mc_settings(_desc.client), mc_connected(false)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::NETWORK_SYSTEM, "Initialising Network System\n");
		
		m_logger.Unindent();
	}



	NetworkSystem::~NetworkSystem()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::NETWORK_SYSTEM, "Shutting Down NetworkSystem\n");
		
		
		if (mc_connected)
		{
			Disconnect();
		}


		m_logger.Unindent();
	}



	NETWORK_SYSTEM_ERROR_CODE NetworkSystem::Update(Registry& _reg)
	{
		if (m_type == NETWORK_SYSTEM_TYPE::SERVER) { return ServerUpdate(_reg); }
		if (m_type == NETWORK_SYSTEM_TYPE::CLIENT) { return ClientUpdate(_reg); }
		return NETWORK_SYSTEM_ERROR_CODE::SUCCESS;
	}

	

	NETWORK_SYSTEM_ERROR_CODE NetworkSystem::ServerUpdate(Registry& _reg)
	{
		m_logger.Indent();
		
		
		if (ms_nextClientIndex != FreeListAllocator::INVALID_INDEX)
		{
			const NETWORK_SYSTEM_ERROR_CODE err{ S_CheckForIncomingConnectionRequests() };
			if (err != NETWORK_SYSTEM_ERROR_CODE::SUCCESS)
			{
				m_logger.Unindent();
				return err;
			}
		}

		NETWORK_SYSTEM_ERROR_CODE err{ S_CheckForIncomingTCPData(_reg) };
		if (err != NETWORK_SYSTEM_ERROR_CODE::SUCCESS)
		{
			m_logger.Unindent();
			return err;
		}

		err = S_CheckForIncomingUDPData(_reg);
		if (err != NETWORK_SYSTEM_ERROR_CODE::SUCCESS)
		{
			m_logger.Unindent();
			return err;
		}


		m_logger.Unindent();
		return NETWORK_SYSTEM_ERROR_CODE::SUCCESS;
	}



	NETWORK_SYSTEM_ERROR_CODE NetworkSystem::S_CheckForIncomingConnectionRequests()
	{
		//Create a new socket at a free client index and check if there's a connection request
		
		sf::TcpSocket& socket{ ms_connectedClientTCPSockets[ms_nextClientIndex] };
		socket.setBlocking(false);
		if (ms_tcpListener.accept(socket) == sf::Socket::Status::Done)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::NETWORK_SYSTEM_SERVER, "Client (address: " + socket.getRemoteAddress()->toString() + ":" + std::to_string(socket.getRemotePort()) + ") connected to the server.\n");

			//Send client index back to client
			sf::Packet outgoingPacket;
			outgoingPacket << ms_nextClientIndex << '\n';
			if (ms_connectedClientTCPSockets[ms_nextClientIndex].send(outgoingPacket) != sf::Socket::Status::Done)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::NETWORK_SYSTEM_SERVER, "Failed to send client index back to client\n");
				ms_connectedClientTCPSockets.erase(ms_nextClientIndex);
				return NETWORK_SYSTEM_ERROR_CODE::SERVER__FAILED_TO_SEND_CLIENT_INDEX_PACKET;
			}

			ms_nextClientIndex = ms_clientIndexAllocator->Allocate();
		}
		else
		{
			ms_connectedClientTCPSockets.erase(ms_nextClientIndex);
		}

		
		return NETWORK_SYSTEM_ERROR_CODE::SUCCESS;
	}



	NETWORK_SYSTEM_ERROR_CODE NetworkSystem::S_CheckForIncomingTCPData(Registry& _reg)
	{
		//In the event of an error, store error code in err rather than returning immediately
		//^Returning immediately could lead to, for example, a client whom was able to repeatedly flood the server with error packets being able to stop data for subsequent client being received
		NETWORK_SYSTEM_ERROR_CODE err{ NETWORK_SYSTEM_ERROR_CODE::SUCCESS };

		
		//Split up the gathering of packets which is very fast from the processing of packets which might be (relatively) much slower
		//This stops the server getting stuck in an infinite loop if packets are being sent faster than they can be processed

		
		//Gather packets
		//Loop through all connected TCP sockets to see if data is being received
		std::unordered_map<ClientIndex, std::vector<sf::Packet>> clientPackets;
		for (std::unordered_map<ClientIndex, sf::TcpSocket>::iterator it{ ms_connectedClientTCPSockets.begin() }; it != ms_connectedClientTCPSockets.end(); ++it)
		{			
			const ClientIndex& index{ it->first };
			
			sf::Packet incomingData;
			while (it->second.receive(incomingData) == sf::Socket::Status::Done)
			{
				clientPackets[index].push_back(incomingData);

				//Ensure client hasn't sent more TCP packets this tick than is allowed
				if (clientPackets[index].size() > ms_settings.maxTCPPacketsPerClientPerTick)
				{
					m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::NETWORK_SYSTEM, "Client (index = " + std::to_string(index) + ", address = " + it->second.getRemoteAddress()->toString() + ":" + std::to_string(it->second.getRemotePort()) + ") attempted to send " + std::to_string(clientPackets[index].size()) + " TCP packets this tick - this exceeds the limit set in ms_settings.maxTCPPacketsPerClientPerTick (" + std::to_string(ms_settings.maxTCPPacketsPerClientPerTick) + ") - disconnecting them\n");
					it = S_DisconnectClient(index);
					clientPackets.erase(index);
					break;
				}
			}
		}

		
		//Process packets
		for (std::unordered_map<ClientIndex, std::vector<sf::Packet>>::iterator it{ clientPackets.begin() }; it != clientPackets.end(); ++it)
		{
			for (sf::Packet& packet : it->second)
			{
				//So that we don't process any packets after a disconnect packet
				bool clientDisconnect{ false };
				
				std::underlying_type_t<PACKET_CODE> underlyingPacketCode;
				packet >> underlyingPacketCode;
				switch (const PACKET_CODE code{ underlyingPacketCode })
				{
				case PACKET_CODE::DISCONNECT:
				{
					m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::NETWORK_SYSTEM_SERVER, "Packet received: DISCONNECT\n");
					S_DisconnectClient(it->first);
					clientDisconnect = true;
					break;
				}
				default:
				{
					m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::NETWORK_SYSTEM_SERVER, "Client sent invalid packet code - code = " + std::to_string(underlyingPacketCode) + " - disconnecting them\n");
					S_DisconnectClient(it->first);
					clientDisconnect = true;
					break;
				}
				}

				//If the client has been disconnected (or an attempt has been made to disconnect the client), don't process any of their other packets for this tick
				if (clientDisconnect)
				{
					it = clientPackets.erase(it);
				}
			}
		}


		return err;
	}



	NETWORK_SYSTEM_ERROR_CODE NetworkSystem::S_CheckForIncomingUDPData(Registry& _reg)
	{
		//In the event of an error, store error code in err rather than returning immediately
		//^Returning immediately could lead to, for example, a client whom was able to repeatedly flood the server with error packets being able to stop data for subsequent client being received
		NETWORK_SYSTEM_ERROR_CODE err{ NETWORK_SYSTEM_ERROR_CODE::SUCCESS };

		
		//Split up the gathering of packets which is very fast from the processing of packets which might be (relatively) much slower
		//This stops the server getting stuck in an infinite loop if packets are being sent faster than they can be processed

		
		//Gather packets
		std::unordered_map<ClientIndex, std::vector<sf::Packet>> clientPackets;
		sf::Packet incomingData;
		std::optional<sf::IpAddress> incomingClientIP;
		unsigned short incomingClientPort;
		while (ms_udpSocket.receive(incomingData, incomingClientIP, incomingClientPort) == sf::Socket::Status::Done)
		{
			//Make sure client is connected through TCP
			if (!ms_connectedClients.contains(incomingClientIP->toString().c_str()))
			{
				m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::NETWORK_SYSTEM, "Client at address " + incomingClientIP->toString() + ":" + std::to_string(incomingClientPort) + " attempted to send UDP packet without being connected through TCP\n");

				//There's not really much we can do in this case since the client isn't connected to us
				//Due to the nature of UDP, this could simply happen because the user sent a UDP packet then a TCP disconnect packet, and the UDP packet wasn't received until after the TCP disconnect packet
				//Because of this, it doesn't necessarily indicate any malicious intent from the client, so just ignore it and continue to the next packet
				continue;
			}
			
			const ClientIndex& index{ ms_connectedClients[incomingClientIP->toString().c_str()] };
			clientPackets[index].push_back(incomingData);

			//Ensure client hasn't sent more UDP packets this tick than is allowed
			if (clientPackets[index].size() > ms_settings.maxUDPPacketsPerClientPerTick)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::NETWORK_SYSTEM, "Client (index = " + std::to_string(index) + ", address = " + incomingClientIP->toString() + ":" + std::to_string(incomingClientPort) + ") attempted to send " + std::to_string(clientPackets[index].size()) + " UDP packets this tick - this exceeds the limit set in ms_settings.maxUDPPacketsPerClientPerTick (" + std::to_string(ms_settings.maxUDPPacketsPerClientPerTick) + ") - disconnecting them\n");
				S_DisconnectClient(index);
				clientPackets.erase(index);
			}
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
				if (!ms_connectedClientUDPPorts.contains(it->first))
				{
					if (code != PACKET_CODE::UDP_PORT)
					{
						m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::NETWORK_SYSTEM, "Client (index = " + std::to_string(it->first) + ", address = " + incomingClientIP->toString() + ":" + std::to_string(incomingClientPort) + ") attempted to send a UDP packet that was not PACKET_CODE::CLIENT_INDEX_FOR_UDP_PORT without being connected through UDP (code = " + std::to_string(underlyingPacketCode) + ") - disconnecting them\n");
						S_DisconnectClient(it->first);
						clientDisconnect = true;
						break;
					}
				}

				switch (code)
				{
				case PACKET_CODE::UDP_PORT:
				{
					m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::NETWORK_SYSTEM_SERVER, "Packet received: UDP_PORT\n");
					ms_connectedClientUDPPorts[it->first] = incomingClientPort;
					break;
				}
				default:
				{
					m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::NETWORK_SYSTEM_SERVER, "Client sent invalid packet code - code = " + std::to_string(underlyingPacketCode) + " - disconnecting them\n");
					S_DisconnectClient(it->first);
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



	std::unordered_map<ClientIndex, sf::TcpSocket>::iterator NetworkSystem::S_DisconnectClient(const ClientIndex _index)
	{
		m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::NETWORK_SYSTEM_SERVER, "Client (index: " + std::to_string(_index) + ") disconnected from the server\n");
		if (ms_connectedClientUDPPorts.contains(_index)) { ms_connectedClientUDPPorts.erase(_index); }
		return ms_connectedClientTCPSockets.erase(ms_connectedClientTCPSockets.find(_index));
	}



	NETWORK_SYSTEM_ERROR_CODE NetworkSystem::ClientUpdate(Registry& _reg)
	{
		return NETWORK_SYSTEM_ERROR_CODE::SUCCESS;
	}



	NETWORK_SYSTEM_ERROR_CODE NetworkSystem::Host(const unsigned short _port)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::NETWORK_SYSTEM_SERVER, "Host() Called For Port " + std::to_string(_port) + "\n");

		
		if (m_type != NETWORK_SYSTEM_TYPE::NONE)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::NETWORK_SYSTEM_SERVER, "Host() called on a NetworkSystem whose m_type != NETWORK_SYSTEM_TYPE::NONE - m_type = " + std::to_string(std::to_underlying(m_type)));
			throw std::runtime_error("");
		}

		ms_address = (ms_settings.type == SERVER_TYPE::LAN ? sf::IpAddress::getLocalAddress() : sf::IpAddress::getPublicAddress());
		ms_port = _port;
		m_type = NETWORK_SYSTEM_TYPE::SERVER;

		
		//Timer for tracking timeout
		Timer timer{ ms_settings.portClaimTimeout };

		
		//Assign TCP listener to port
		//Repeatedly try to claim the port until successful or timeout
		ms_tcpListener.setBlocking(true);
		while (ms_tcpListener.listen(ms_port) != sf::Socket::Status::Done)
		{
			if (timer.IsComplete())
			{
				//Timeout reached
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::NETWORK_SYSTEM_SERVER, "Timeout reached while attempting to bind TCP listener to port\n");
				m_logger.Unindent();
				return NETWORK_SYSTEM_ERROR_CODE::SERVER__TCP_LISTENER_PORT_CLAIM_TIME_OUT;
			}
			sf::sleep(sf::milliseconds(20));
			timer.Update();
		}
		m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::NETWORK_SYSTEM_SERVER, "TCP listener successfully bound to port\n");
		ms_tcpListener.setBlocking(false);

		
		//Assign UDP socket to port
		//Repeatedly try to claim the port until successful or timeout
		timer.Reset(ms_settings.portClaimTimeout);
		ms_udpSocket.setBlocking(true);
		while (ms_udpSocket.bind(ms_port) != sf::Socket::Status::Done)
		{
			if (timer.IsComplete())
			{
				//Timeout reached
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::NETWORK_SYSTEM_SERVER, "Timeout reached while attempting to bind UDP socket to port\n");
				m_logger.Unindent();
				return NETWORK_SYSTEM_ERROR_CODE::SERVER__UDP_SOCKET_PORT_CLAIM_TIME_OUT;
			}
			sf::sleep(sf::milliseconds(20));
			timer.Update();
		}
		m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::NETWORK_SYSTEM_SERVER, "UDP socket successfully bound to port\n");
		ms_udpSocket.setBlocking(false);

		m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::NETWORK_SYSTEM_SERVER, std::string(ms_settings.type == SERVER_TYPE::LAN ? "LAN" : "WAN") + " server set up on " + ms_address.value().toString() + ":" + std::to_string(ms_port) + "\n");

		
		ms_nextClientIndex = ms_clientIndexAllocator->Allocate();
		m_logger.Unindent();
		return NETWORK_SYSTEM_ERROR_CODE::SUCCESS;
	}



	NETWORK_SYSTEM_ERROR_CODE NetworkSystem::Connect(const char* _ip, const unsigned short _port)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::NETWORK_SYSTEM_CLIENT, "Connect() Called For Address " + std::string(_ip) + ":" + std::to_string(_port) + "\n");

		mc_serverPort = _port;


		if (m_type != NETWORK_SYSTEM_TYPE::NONE)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::NETWORK_SYSTEM_CLIENT, "Connect() called on a NetworkSystem whose m_type != NETWORK_SYSTEM_TYPE::NONE - m_type = " + std::to_string(std::to_underlying(m_type)));
			throw std::runtime_error("");
		}
		m_type = NETWORK_SYSTEM_TYPE::CLIENT;
			
		
		mc_serverAddress = sf::IpAddress::resolve(_ip);
		if (!mc_serverAddress.has_value())
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::NETWORK_SYSTEM_CLIENT, "Connect() - _ip is invalid - _ip = " + std::string(_ip) + "\n");
			m_logger.Unindent();
			return NETWORK_SYSTEM_ERROR_CODE::CLIENT__INVALID_SERVER_IP_ADDRESS;
		}

		
		//Repeatedly try to connect to the server until successful or timeout
		Timer timer{ mc_settings.serverConnectTimeout };
		mc_tcpSocket.setBlocking(true);
		while (mc_tcpSocket.connect(mc_serverAddress.value(), _port) != sf::Socket::Status::Done)
		{
			if (timer.IsComplete())
			{
				//Timeout reached
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::NETWORK_SYSTEM_CLIENT, "Timeout reached while attempting to connect to server\n");
				m_logger.Unindent();
				return NETWORK_SYSTEM_ERROR_CODE::CLIENT__SERVER_CONNECTION_TIMED_OUT;
			}
			sf::sleep(sf::milliseconds(20));
			timer.Update();
		}
		m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::NETWORK_SYSTEM_CLIENT, "Successfully connected to server\n");

		
		mc_tcpSocket.setBlocking(false);


		//Get client index from server
		timer.Reset(mc_settings.serverClientIndexPacketTimeout);
		sf::Packet incomingData;
		while (mc_tcpSocket.receive(incomingData) != sf::Socket::Status::Done)
		{
			if (timer.IsComplete())
			{
				//Timeout reached
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::NETWORK_SYSTEM_CLIENT, "Timeout reached while attempting to get client index from server\n");
				m_logger.Unindent();
				return NETWORK_SYSTEM_ERROR_CODE::CLIENT__CLIENT_INDEX_PACKET_RETRIEVAL_TIMED_OUT;
			}
			sf::sleep(sf::milliseconds(20));
			timer.Update();
		}
		incomingData >> mc_index;
		m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::NETWORK_SYSTEM_CLIENT, "Successfully received client index packet from server. Client index = " + std::to_string(mc_index) + "\n");


		//Send packet to server over UDP (so the server can get the udp socket's port)
		//Use blocking so as to not send a bunch of packets
		sf::Packet outgoingPacket;
		outgoingPacket << std::to_underlying(PACKET_CODE::UDP_PORT);
		if (mc_udpSocket.send(outgoingPacket, mc_serverAddress.value(), mc_serverPort) != sf::Socket::Status::Done)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::NETWORK_SYSTEM_CLIENT, "Failed to send UDP port packet to server\n");
			m_logger.Unindent();
			return NETWORK_SYSTEM_ERROR_CODE::CLIENT__FAILED_TO_SEND_UDP_PORT_PACKET;
		}
		m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::NETWORK_SYSTEM_CLIENT, "Successfully sent UDP port packet to server\n");
		mc_udpSocket.setBlocking(false);

		mc_connected = true;
		m_logger.Unindent();
		return NETWORK_SYSTEM_ERROR_CODE::SUCCESS;
	}



	NETWORK_SYSTEM_ERROR_CODE NetworkSystem::Disconnect()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::NETWORK_SYSTEM_CLIENT, "Disconnecting From Server.\n");

		
		if (m_type != NETWORK_SYSTEM_TYPE::CLIENT)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::NETWORK_SYSTEM_CLIENT, "Disconnect() - m_type != NETWORK_SYSTEM_TYPE::CLIENT - m_type = " + std::to_string(std::to_underlying(m_type)));
			throw std::runtime_error("");
		}

		if (!mc_connected)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::NETWORK_SYSTEM_CLIENT, "Disconnect() - client is not currently connected to a server");
			throw std::runtime_error("");
		}

		
		mc_tcpSocket.setBlocking(true);
		sf::Packet outgoingPacket;
		outgoingPacket << std::to_underlying(PACKET_CODE::DISCONNECT);
		if (mc_tcpSocket.send(outgoingPacket) != sf::Socket::Status::Done)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::NETWORK_SYSTEM_CLIENT, "Disconnect() - Failed to disconnect from server\n");
			m_logger.Unindent();
			return NETWORK_SYSTEM_ERROR_CODE::CLIENT__FAILED_TO_DISCONNECT_FROM_SERVER;
		}
		m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::NETWORK_SYSTEM_CLIENT, "Successfully disconnected from server\n");

		mc_connected = false;
		m_logger.Unindent();
		return NETWORK_SYSTEM_ERROR_CODE::SUCCESS;
	}

}