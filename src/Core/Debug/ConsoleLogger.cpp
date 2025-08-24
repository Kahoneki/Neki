#include "ConsoleLogger.h"
#include <iomanip>
#include <iostream>

namespace NK
{

	void ConsoleLogger::Log(LOGGER_CHANNEL _channel, LOGGER_LAYER _layer, const std::string& _message) const
	{
		LogImpl(_channel, _layer, _message, true);
	}



	void ConsoleLogger::RawLog(LOGGER_CHANNEL _channel, LOGGER_LAYER _layer, const std::string& _message) const
	{
		LogImpl(_channel, _layer, _message, false);
	}



	void ConsoleLogger::LogImpl(LOGGER_CHANNEL _channel, LOGGER_LAYER _layer, const std::string& _message, bool _formatted) const
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
			std::cerr << std::endl << "ERR::Logger::Log::DEFAULT_SEVERITY_CASE_REACHED::CHANNEL_WAS_" << std::to_string(static_cast<std::underlying_type_t<LOGGER_CHANNEL>>(_channel)) << "::FALLING_BACK_TO_DEFAULT_CHANNEL" << std::endl;
			channelStr = "[DEFAULT]";
			colourCode = COLOUR_RESET;
			break;
		}

		//std::cerr for errors, std::cout for everything else
		std::ostream& stream{ (_channel == LOGGER_CHANNEL::ERROR) ? std::cerr : std::cout };
		stream << colourCode;
		if (_formatted)
		{
			stream << std::left << std::setw(static_cast<std::underlying_type_t<LOGGER_WIDTH>>(LOGGER_WIDTH::CHANNEL)) << channelStr <<
			std::left << std::setw(static_cast<std::underlying_type_t<LOGGER_WIDTH>>(LOGGER_WIDTH::LAYER)) << LayerToString(_layer);
		}
		stream << _message << COLOUR_RESET;
	}
}