#pragma once

#include "LoggerConfig.h"

#include <Types/NekiTypes.h>

#include <string>


namespace NK
{
	
	class ILogger
	{
	public:
		virtual ~ILogger() = default;

		//Logs _message from _layer to the _channel channel (if the channel is enabled for the specified layer)
		//Newline characters are not included by this function, they should be contained within _message
		//Optionally, pass in an indentation level to override the member
		inline void Log(LOGGER_CHANNEL _channel, LOGGER_LAYER _layer, const std::string& _message, std::int32_t _indentationLevel=INT32_MAX) const
		{
			//Use non-virtual-interface pattern due to default parameter
			LogImpl(_channel, _layer, _message, _indentationLevel);
		}
		
		//Same as Log(), but doesn't include any formatting
		inline void RawLog(LOGGER_CHANNEL _channel, LOGGER_LAYER _layer, const std::string& _message, std::int32_t _indentationLevel=INT32_MAX) const
		{
			//Use non-virtual-interface pattern due to default parameter
			RawLogImpl(_channel, _layer, _message, _indentationLevel);
		};

		//Calls Indent(), Log(), Unindent()
		inline void IndentLog(LOGGER_CHANNEL _channel, LOGGER_LAYER _layer, const std::string& _message)
		{
			Indent();
			Log(_channel, _layer, _message, indentationLevel);
			Unindent();
		}

		//Calls Indent(), RawLog(), Unindent()
		inline void IndentRawLog(LOGGER_CHANNEL _channel, LOGGER_LAYER _layer, const std::string& _message)
		{
			Indent();
			RawLog(_channel, _layer, _message, indentationLevel);
			Unindent();
		}

		//Increment the indentation level
		inline void Indent() { ++indentationLevel; }

		//Decrement the indentation level
		//indentation value can be negative and it will be clamped to 0 when outputting - this lets you have an "indentation buffer region"
		//If indentationLevel is -1, 0-indented logs will be at the same indentation level as 1-indented logs (0 indents)
		inline void Unindent() { --indentationLevel; }


		inline const LoggerConfig& GetLoggerConfig() const { return m_config; }
		
		static std::string LayerToString(LOGGER_LAYER _layer);


		//ANSI colour codes
		static constexpr const char* COLOUR_RED   { "\033[31m" };
		static constexpr const char* COLOUR_GREEN { "\033[32m" };
		static constexpr const char* COLOUR_YELLOW{ "\033[33m" };
		static constexpr const char* COLOUR_CYAN  { "\033[36m" };
		static constexpr const char* COLOUR_WHITE { "\033[97m" }; //37 (regular white) is quite dark, use 97 (high intensity white)
		static constexpr const char* COLOUR_RESET { "\033[0m"  };

		
	protected:
		explicit ILogger(const LoggerConfig& _config);

		//NVI impls
		virtual void LogImpl(LOGGER_CHANNEL _channel, LOGGER_LAYER _layer, const std::string& _message, std::int32_t _indentationLevel) const = 0;
		virtual void RawLogImpl(LOGGER_CHANNEL _channel, LOGGER_LAYER _layer, const std::string& _message, std::int32_t _indentationLevel) const = 0;
		
		//For older Windows systems
		//Attempt to enable ANSI support for access to ANSI colour codes
		//Returns success status
		static bool EnableAnsiSupport();
		

		//For output-formatting
		enum class LOGGER_WIDTH : std::uint32_t
		{
			CHANNEL = 10,
			LAYER = 25,
		};
		int32_t indentationLevel{ 0 }; //Stores the amount of indents currently active - controlled by Indent() and Unindent() - can be negative, acting as a buffer region
		
		const LoggerConfig m_config;
	};
}