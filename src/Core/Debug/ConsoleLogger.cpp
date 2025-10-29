#include "ConsoleLogger.h"

#include <iostream>
#include <iomanip>


namespace NK
{

	void ConsoleLogger::LogRawLogImpl(LOGGER_CHANNEL _channel, LOGGER_LAYER _layer, const std::string& _message, std::int32_t _indentationValue, bool _formatted) const
	{
		//Check if channel is enabled for the specified layer
		//If it's not, just early return
		if ((m_config.GetChannelBitfieldForLayer(_layer) & _channel) == LOGGER_CHANNEL::NONE) { return; }

		std::string channelStr;
		std::string colourCode;

		switch (_channel)
		{
		case LOGGER_CHANNEL::INFO:
			channelStr = "[INFO]";
			colourCode = COLOUR_WHITE;
			break;
		case LOGGER_CHANNEL::HEADING:
			channelStr = "[HEADING]";
			colourCode = COLOUR_CYAN;
			break;
		case LOGGER_CHANNEL::WARNING:
			channelStr = "[WARNING]";
			colourCode = COLOUR_YELLOW;
			break;
		case LOGGER_CHANNEL::ERROR:
			channelStr = "[ERROR]";
			colourCode = COLOUR_RED;
			break;
		case LOGGER_CHANNEL::SUCCESS:
			channelStr = "[SUCCESS]";
			colourCode = COLOUR_GREEN;
			break;
		default:
			std::cerr << std::endl << "ERR::Logger::Log::DEFAULT_SEVERITY_CASE_REACHED::CHANNEL_WAS_" << std::to_string(std::to_underlying(_channel)) << "::FALLING_BACK_TO_DEFAULT_CHANNEL" << std::endl;
			channelStr = "[DEFAULT]";
			colourCode = COLOUR_RESET;
			break;
		}

		//Add spaces to start of message based on indentationLevel or _indentationValue override if it has been provided (!INT32_MAX)
		constexpr std::uint32_t spacesPerIndent{ 2 };
		std::string indentedMessage{ _message };
		indentedMessage.insert(0, std::max((_indentationValue == INT32_MAX ? indentationLevel : _indentationValue), 0) * spacesPerIndent, ' '); //Clamp indentation level to 0
		
		std::reference_wrapper<std::ostream> stream{ std::cout };
		if (_channel == LOGGER_CHANNEL::ERROR)
		{
			stream.get() << std::flush;
			stream = std::cerr;
		}
		stream.get() << colourCode;
		if (_formatted)
		{
			stream.get() << std::left << std::setw(std::to_underlying(LOGGER_WIDTH::CHANNEL)) << channelStr <<
			std::left << std::setw(std::to_underlying(LOGGER_WIDTH::LAYER)) << LayerToString(_layer);
		}
		stream.get() << indentedMessage << COLOUR_RESET;

		if (_channel == LOGGER_CHANNEL::ERROR) { stream.get() << std::flush; }
	}
	
}