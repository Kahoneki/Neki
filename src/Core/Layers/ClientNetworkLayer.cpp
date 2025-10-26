#include "ClientNetworkLayer.h"

#include <Core/Utils/Timer.h>


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
		sf::Packet outgoingPacket;
		outgoingPacket << std::to_underlying(PACKET_CODE::UDP_PORT);
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
		

		if (m_state == CLIENT_STATE::CONNECTED)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::CLIENT_NETWORK_LAYER, "Disconnect() - client is not currently connected to a server - m_state = " + std::to_string(std::to_underlying(m_state)));
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

}