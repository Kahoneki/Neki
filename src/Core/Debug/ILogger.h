#pragma once
#include <string>
#include "LoggerConfig.h"

namespace NK
{
	class ILogger
	{
	public:
		virtual ~ILogger() = default;

		//Logs _message from _layer to the _channel channel (if the channel is enabled for the specified layer)
		//Newline characters are not included by this function, they should be contained within _message
		virtual void Log(LOGGER_CHANNEL _channel, LOGGER_LAYER _layer, const std::string& _message) const = 0;
		
		//Same as Log(), but doesn't include any formatting
		virtual void RawLog(LOGGER_CHANNEL _channel, LOGGER_LAYER _layer, const std::string& _message) const = 0;
		
	protected:
		explicit ILogger(const LoggerConfig& _config);
		
		//For older Windows systems
		//Attempt to enable ANSI support for access to ANSI colour codes
		//Returns success status
		static bool EnableAnsiSupport();

		static std::string LayerToString(LOGGER_LAYER _layer);

		
		//ANSI colour codes
		static constexpr const char* COLOUR_RED   { "\033[31m" };
		static constexpr const char* COLOUR_GREEN { "\033[32m" };
		static constexpr const char* COLOUR_YELLOW{ "\033[33m" };
		static constexpr const char* COLOUR_CYAN  { "\033[36m" };
		static constexpr const char* COLOUR_WHITE { "\033[97m" }; //37 (regular white) is quite dark, use 97 (high intensity white)
		static constexpr const char* COLOUR_RESET { "\033[0m"  };

		//For output-formatting
		enum class LOGGER_WIDTH : std::uint32_t
		{
			CHANNEL = 10,
			LAYER = 25,
		};
		
		const LoggerConfig m_config;
	};
}