#include "ServerNetworkLayer.h"

#include <Components/CInput.h>
#include <Components/CTransform.h>
#include <Core/Utils/Timer.h>

#include <cereal/archives/binary.hpp>


namespace NK
{

	ServerNetworkLayer::ServerNetworkLayer(Registry& _reg, const ServerNetworkLayerDesc& _desc)
	: ILayer(_reg), m_desc(_desc), m_state(SERVER_STATE::NOT_HOSTING), m_clientIndexAllocator(NK_NEW(FreeListAllocator, m_desc.maxClients))
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



	void ServerNetworkLayer::Update()
	{
		if (Context::GetLayerUpdateState() == LAYER_UPDATE_STATE::PRE_APP) { PreAppUpdate(); }
		else { PostAppUpdate(); }
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
		//Create a temporary socket to accept the new connection
		sf::TcpSocket socket;
		socket.setBlocking(false);
		if (m_tcpListener.accept(socket) == sf::Socket::Status::Done)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::SERVER_NETWORK_LAYER, "Client (address: " + socket.getRemoteAddress()->toString() + ":" + std::to_string(socket.getRemotePort()) + ") connected to the server.\n");

			//Connection was successful, add to maps
			m_connectedClientTCPSockets[m_nextClientIndex] = std::move(socket);
			const UniqueAddress address{ m_connectedClientTCPSockets[m_nextClientIndex].getRemoteAddress()->toString(), m_connectedClientTCPSockets[m_nextClientIndex].getRemotePort() };
			m_connectedClientTCPAddresses[m_nextClientIndex] = address;
			m_rev_connectedClientTCPAddresses[address] = m_nextClientIndex;
			
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



	NETWORK_LAYER_ERROR_CODE ServerNetworkLayer::CheckForIncomingTCPData()
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
			//So that we don't try to process any more of a client's packets after disconnecting them
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
				case PACKET_CODE::EVENT:
				{
					m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::SERVER_NETWORK_LAYER, "Packet received: EVENT\n");
					m_tcpEventHandler->HandleEvent(packet);
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



	NETWORK_LAYER_ERROR_CODE ServerNetworkLayer::CheckForIncomingUDPData()
	{
		//In the event of an error, store error code in err rather than returning immediately
		//^Returning immediately could lead to, for example, a client whom was able to repeatedly flood the server with error packets being able to stop data for subsequent client being received
		NETWORK_LAYER_ERROR_CODE err{ NETWORK_LAYER_ERROR_CODE::SUCCESS };

		
		//Split up the gathering of packets which is very fast from the processing of packets which might be (relatively) much slower
		//This stops the server getting stuck in an infinite loop if packets are being sent faster than they can be processed
		std::unordered_map<ClientIndex, std::vector<sf::Packet>> clientPackets;

		
		//Gather packets
		sf::Packet incomingData;
		std::optional<sf::IpAddress> incomingClientIP;
		unsigned short incomingClientPort;
		m_udpSocket.setBlocking(false);
		while (m_udpSocket.receive(incomingData, incomingClientIP, incomingClientPort) == sf::Socket::Status::Done)
		{
			sf::Packet packetCopy{ incomingData };
			std::underlying_type_t<PACKET_CODE> codeValue;
			packetCopy >> codeValue;
			const PACKET_CODE code{ static_cast<PACKET_CODE>(codeValue) };
			if (code == PACKET_CODE::UDP_PORT)
			{
				ClientIndex index;
				packetCopy >> index;
				if (m_connectedClientTCPSockets.contains(index))
				{
					m_connectedClientUDPAddresses[index] = { incomingClientIP->toString(), incomingClientPort };
					m_rev_connectedClientUDPAddresses[m_connectedClientUDPAddresses[index]] = index;
					m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::SERVER_NETWORK_LAYER, "Registered UDP endpoint for client " + std::to_string(index) + " (address: " + incomingClientIP->toString() + ":" + std::to_string(incomingClientPort) + '\n');
				}
			}
			else
			{
				if (!m_rev_connectedClientUDPAddresses.contains({ incomingClientIP->toString(), incomingClientPort }))
				{
					continue;
				}
				ClientIndex index{ m_rev_connectedClientUDPAddresses.at({ incomingClientIP->toString(), incomingClientPort }) };
				clientPackets[index].push_back(std::move(incomingData));
			}
		}
		
		
		//Process packets
		for (std::unordered_map<ClientIndex, std::vector<sf::Packet>>::iterator it{ clientPackets.begin() }; it != clientPackets.end(); ++it)
		{
			//So that we don't process anymore of a client's packets after they're disconnected
			bool clientDisconnect{ false };

			for (sf::Packet& packet : it->second)
			{
				std::underlying_type_t<PACKET_CODE> underlyingPacketCode;
				packet >> underlyingPacketCode;
				const PACKET_CODE code{ underlyingPacketCode };

				switch (code)
				{
				case PACKET_CODE::INPUT:
				{
					m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::SERVER_NETWORK_LAYER, std::to_string(it->first) + ": Packet received: INPUT\n");
					DecodeAndApplyInput(packet);
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

		m_rev_connectedClientTCPAddresses.erase(m_connectedClientTCPAddresses[_index]);
		m_connectedClientTCPAddresses.erase(_index);
		
		if (m_connectedClientUDPAddresses.contains(_index))
		{
			m_rev_connectedClientUDPAddresses.erase(m_connectedClientUDPAddresses[_index]);
			m_connectedClientUDPAddresses.erase(_index);
		}
		
		m_clientIndexAllocator->Free(_index);

		return m_connectedClientTCPSockets.erase(m_connectedClientTCPSockets.find(_index));
	}



	void ServerNetworkLayer::PreAppUpdate()
	{
		//If the server was full, try to allocate a new index because a slot might have opened up from a client disconnecting
		if (m_nextClientIndex == FreeListAllocator::INVALID_INDEX)
		{
			m_nextClientIndex = m_clientIndexAllocator->Allocate();
		}
		
		if (m_nextClientIndex != FreeListAllocator::INVALID_INDEX)
		{
			const NETWORK_LAYER_ERROR_CODE err{ CheckForIncomingConnectionRequests() };
			if (err != NETWORK_LAYER_ERROR_CODE::SUCCESS)
			{
				m_logger.Unindent();
				return;
			}
		}

		NETWORK_LAYER_ERROR_CODE err{ CheckForIncomingTCPData() };
		if (err != NETWORK_LAYER_ERROR_CODE::SUCCESS)
		{
			m_logger.Unindent();
			return;
		}

		err = CheckForIncomingUDPData();
		if (err != NETWORK_LAYER_ERROR_CODE::SUCCESS)
		{
			m_logger.Unindent();
			return;
		}
	}



	void ServerNetworkLayer::PostAppUpdate()
	{
		//Serialise all CTransforms and send them to the clients
		std::queue<NetworkTransformData> transformComponents;
		for (auto&& [transform] : m_reg.get().View<CTransform>())
		{
			NetworkTransformData data{};
			data.entity = m_reg.get().GetEntity(transform);
			data.pos = transform.GetLocalPosition();
			data.rot = transform.GetLocalRotation();
			data.scale = transform.GetLocalScale();
			transformComponents.push(data);
		}
		if (transformComponents.empty())
		{
			m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::SERVER_NETWORK_LAYER, "No `CTransform`s found in registry\n");
			return;
		}
		std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
		{
			cereal::BinaryOutputArchive archive(ss);
			archive(transformComponents);
		}
		sf::Packet outgoingPacket;
		outgoingPacket << std::to_underlying(PACKET_CODE::TRANSFORM);
		outgoingPacket.append(ss.str().c_str(), ss.str().size());
		for (std::unordered_map<ClientIndex, UniqueAddress>::iterator it{ m_connectedClientUDPAddresses.begin() }; it != m_connectedClientUDPAddresses.end(); ++it)
		{
			if (m_udpSocket.send(outgoingPacket, sf::IpAddress::resolve(it->second.first).value(), it->second.second) == sf::Socket::Status::Error)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::CLIENT_NETWORK_LAYER, "Failed to send UDP packet to server\n");
			}
		}
	}



	void ServerNetworkLayer::DecodeAndApplyInput(const sf::Packet& _packet) const
	{
		//Deserialise all CInputs and apply them
		std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
		ss.write(static_cast<const char*>(_packet.getData()) + _packet.getReadPosition(), _packet.getDataSize());
		std::queue<NetworkInputData> inputData;
		{
			cereal::BinaryInputArchive archive(ss);
			archive(inputData);
		}

		while (!inputData.empty())
		{
			const NetworkInputData& data{ inputData.back() };
			m_reg.get().GetComponent<CInput>(data.entity).actionStates = data.actionStates;
			inputData.pop();
		}
	}

}