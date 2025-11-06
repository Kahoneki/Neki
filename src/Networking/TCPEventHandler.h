#pragma once

#include <SFML/Network.hpp>


namespace NK
{
	
	//Pure virtual abstract base class for handling TCP events
	//Derive from this class and overload the HandleEvent() function to handle all events (that could be sent over the network) for your project
	//Call ServerNetworkLayer::SetTCPEventHandler(), passing in your derived class
	class TCPEventHandler
	{
	public:
		//_packet structure: type registry constant for the event type then the event data itself
		virtual void HandleEvent(sf::Packet _packet) = 0;
	};

}