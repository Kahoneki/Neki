#include "ClientNetworkLayer.h"

#include <Components/CInput.h>
#include <Components/CNetworkSync.h>
#include <Components/CTransform.h>
#include <Core/Utils/Timer.h>

#include <queue>
#include <cereal/archives/binary.hpp>


namespace NK
{

	ClientNetworkLayer::ClientNetworkLayer(Registry& _reg, const ClientNetworkLayerDesc& _desc)
	: ILayer(_reg), m_desc(_desc), m_state(CLIENT_STATE::DISCONNECTED)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::CLIENT_NETWORK_LAYER, "Initialising Client Network Layer\n");
		
		m_logger.Unindent();
	}



	ClientNetworkLayer::~ClientNetworkLayer()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::CLIENT_NETWORK_LAYER, "Shutting Down Client Network Layer\n");
		
		
		if (m_state == CLIENT_STATE::CONNECTED)
		{
			Disconnect();
		}


		m_logger.Unindent();
	}



	void ClientNetworkLayer::Update()
	{
		if (Context::GetLayerUpdateState() == LAYER_UPDATE_STATE::PRE_APP) { PreAppUpdate(); }
		else { PostAppUpdate(); }
	}

	

	NETWORK_LAYER_ERROR_CODE ClientNetworkLayer::Connect(const char* _ip, const unsigned short _port)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::CLIENT_NETWORK_LAYER, "Connect() Called For Address " + std::string(_ip) + ":" + std::to_string(_port) + "\n");

		
		if (m_state != CLIENT_STATE::DISCONNECTED)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::CLIENT_NETWORK_LAYER, "Connect() called on a ClientNetworkLayer whose m_state != CLIENT_STATE::DISCONNECTED - m_state = " + std::to_string(std::to_underlying(m_state)));
			throw std::runtime_error("");
		}
		
		m_serverAddress = sf::IpAddress::resolve(_ip);
		if (!m_serverAddress.has_value())
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::CLIENT_NETWORK_LAYER, "Connect() - _ip is invalid - _ip = " + std::string(_ip) + "\n");
			m_logger.Unindent();
			return NETWORK_LAYER_ERROR_CODE::CLIENT__INVALID_SERVER_IP_ADDRESS;
		}
		m_serverPort = _port;


		//Initiate 3-step handshake with server - if successful, client state will be moved to CONNECTED
		m_state = CLIENT_STATE::CONNECTING;

		
		//Step 1: Repeatedly try to establish a connection until successful or timeout
		Timer timer{ m_desc.serverConnectTimeout };
		while (m_tcpSocket.connect(m_serverAddress.value(), _port) != sf::Socket::Status::Done)
		{
			if (timer.IsComplete())
			{
				//Timeout reached
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::CLIENT_NETWORK_LAYER, "Timeout reached while attempting to contact the server\n");
				m_logger.Unindent();
				return NETWORK_LAYER_ERROR_CODE::CLIENT__SERVER_CONNECTION_TIMED_OUT;
			}
			sf::sleep(sf::milliseconds(20));
			timer.Update();
		}
		m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::CLIENT_NETWORK_LAYER, "Successfully contacted server\n");
		m_tcpSocket.setBlocking(false);


		//Step 2: Get client index from server
		timer.Reset(m_desc.serverClientIndexPacketTimeout);
		sf::Packet incomingData;
		while (m_tcpSocket.receive(incomingData) != sf::Socket::Status::Done)
		{
			if (timer.IsComplete())
			{
				//Timeout reached
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::CLIENT_NETWORK_LAYER, "Timeout reached while attempting to get client index from server\n");
				m_logger.Unindent();
				return NETWORK_LAYER_ERROR_CODE::CLIENT__CLIENT_INDEX_PACKET_RETRIEVAL_TIMED_OUT;
			}
			sf::sleep(sf::milliseconds(20));
			timer.Update();
		}
		incomingData >> m_index;
		m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::CLIENT_NETWORK_LAYER, "Successfully received client index packet from server. Client index = " + std::to_string(m_index) + "\n");


		//Step 3: Send packet to server over UDP (so the server can get the udp socket's port)
		//Use blocking so as to not send a bunch of packets
		if (m_udpSocket.bind(sf::Socket::AnyPort) != sf::Socket::Status::Done)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::CLIENT_NETWORK_LAYER, "Failed to bind UDP port\n");
			m_logger.Unindent();
			return NETWORK_LAYER_ERROR_CODE::CLIENT__FAILED_TO_BIND_UDP_PORT;
		}
		sf::Packet outgoingPacket;
		outgoingPacket << std::to_underlying(PACKET_CODE::UDP_PORT) << m_index;
		if (m_udpSocket.send(outgoingPacket, m_serverAddress.value(), m_serverPort) != sf::Socket::Status::Done)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::CLIENT_NETWORK_LAYER, "Failed to send UDP port packet to server\n");
			m_logger.Unindent();
			return NETWORK_LAYER_ERROR_CODE::CLIENT__FAILED_TO_SEND_UDP_PORT_PACKET;
		}
		m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::CLIENT_NETWORK_LAYER, "Successfully sent UDP port packet to server\n");
		m_udpSocket.setBlocking(false);


		//3-step handshake complete, move client to connected state
		m_state = CLIENT_STATE::CONNECTED;
		
		
		m_logger.Unindent();
		return NETWORK_LAYER_ERROR_CODE::SUCCESS;
	}



	NETWORK_LAYER_ERROR_CODE ClientNetworkLayer::Disconnect()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::CLIENT_NETWORK_LAYER, "Disconnecting From Server.\n");
		

		if (m_state != CLIENT_STATE::CONNECTED)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::CLIENT_NETWORK_LAYER, "Disconnect() - client is not currently connected to a server - m_state = " + std::to_string(std::to_underlying(m_state)) + "\n");
			throw std::runtime_error("");
		}


		m_state = CLIENT_STATE::DISCONNECTING;
		m_tcpSocket.setBlocking(true);
		sf::Packet outgoingPacket;
		outgoingPacket << std::to_underlying(PACKET_CODE::DISCONNECT);
		if (m_tcpSocket.send(outgoingPacket) != sf::Socket::Status::Done)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::CLIENT_NETWORK_LAYER, "Disconnect() - Failed to disconnect from server\n");
			m_logger.Unindent();
			return NETWORK_LAYER_ERROR_CODE::CLIENT__FAILED_TO_DISCONNECT_FROM_SERVER;
		}
		m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::CLIENT_NETWORK_LAYER, "Successfully disconnected from server\n");

		
		m_state = CLIENT_STATE::DISCONNECTED;
		m_logger.Unindent();
		return NETWORK_LAYER_ERROR_CODE::SUCCESS;
	}



	void ClientNetworkLayer::PreAppUpdate()
	{
		//Serialise all CInputs and send them to the server over UDP
		std::queue<NetworkInputData> inputComponents;
		for (auto&& [input] : m_reg.get().View<CInput>())
		{
			NetworkInputData data{};
			data.entity = m_reg.get().GetEntity(input);
			data.actionStates = input.actionStates;
			inputComponents.push(data);
		}
		if (inputComponents.empty())
		{
			m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::CLIENT_NETWORK_LAYER, "No `CInput`s found in registry\n");
			return;
		}
		std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
		{
			cereal::BinaryOutputArchive archive(ss);
			archive(inputComponents);
		}
		sf::Packet outgoingPacket;
		outgoingPacket << std::to_underlying(PACKET_CODE::INPUT);
		outgoingPacket.append(ss.str().c_str(), ss.str().size());
		if (m_udpSocket.send(outgoingPacket, m_serverAddress.value(), m_serverPort) == sf::Socket::Status::Error)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::CLIENT_NETWORK_LAYER, "Failed to send UDP packet to server\n");
		}


		//Send all enqueued event packets to the server over TCP
		while (!m_tcpEventQueue.empty())
		{
			sf::Packet packet{ std::move(m_tcpEventQueue.front()) };
			if (m_tcpSocket.send(outgoingPacket) == sf::Socket::Status::Error)
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::CLIENT_NETWORK_LAYER, "Failed to send TCP event packet to server\n");
			}

			m_tcpEventQueue.pop();
		}
	}



	void ClientNetworkLayer::PostAppUpdate()
	{
		sf::Packet incomingData;
		
		
		//TCP
		m_tcpSocket.setBlocking(false);
		while (m_tcpSocket.receive(incomingData) == sf::Socket::Status::Done)
		{
			std::underlying_type_t<PACKET_CODE> codeValue;
			incomingData >> codeValue;
			const PACKET_CODE code{ static_cast<PACKET_CODE>(codeValue) };
			switch (code)
			{
			case PACKET_CODE::ENTITY_SPAWN:
			{
				
			}
			}
		}
		
		
		//UDP
		std::optional<sf::IpAddress> incomingClientIP;
		unsigned short incomingClientPort;
		m_udpSocket.setBlocking(false);
		while (m_udpSocket.receive(incomingData, incomingClientIP, incomingClientPort) == sf::Socket::Status::Done)
		{
			if ((incomingClientIP != m_serverAddress) || (incomingClientPort != m_serverPort)) { continue; }
			std::underlying_type_t<PACKET_CODE> codeValue;
			incomingData >> codeValue;
			const PACKET_CODE code{ static_cast<PACKET_CODE>(codeValue) };
			switch (code)
			{
			case PACKET_CODE::TRANSFORM:
			{
				//Deserialise all CTransforms and apply them
				std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
				ss.write(static_cast<const char*>(incomingData.getData()) + incomingData.getReadPosition(), incomingData.getDataSize());
				std::queue<NetworkTransformData> transformData;
				{
					cereal::BinaryInputArchive archive(ss);
					archive(transformData);
				}

				while (!transformData.empty())
				{
					const NetworkTransformData& data{ transformData.back() };
					CTransform& trans{ m_reg.get().GetComponent<CTransform>(data.entity) };
					trans.SetLocalPosition(data.pos);
					trans.SetLocalRotation(data.rot);
					trans.SetLocalScale(data.scale);
					transformData.pop();
				}
				break;
			}
			default:
			{
				m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::SERVER_NETWORK_LAYER, "Server sent invalid packet code - code = " + std::to_string(codeValue) + "\n");
				break;
			}
			}
		}
	}

}
