#pragma once
#include "ILogger.h"

namespace NK
{

	class ConsoleLogger final : public ILogger
	{
	public:
		explicit ConsoleLogger(const LoggerConfig& _config) : ILogger(_config) {}


	private:
		//Logs _message from _layer to the _channel channel (if the channel is enabled for the specified layer)
		//Newline characters are not included by this function, they should be contained within _message
		//Optionally, pass in an indent value to override the tracked one
		virtual inline void LogImpl(LOGGER_CHANNEL _channel, LOGGER_LAYER _layer, const std::string& _message, std::int32_t _indentationValue) const override
		{
			LogRawLogImpl(_channel, _layer, _message, _indentationValue, true);
		}

		//Same as Log(), but doesn't include any formatting
		virtual inline void RawLogImpl(LOGGER_CHANNEL _channel, LOGGER_LAYER _layer, const std::string& _message, std::int32_t _indentationValue) const override
		{
			LogRawLogImpl(_channel, _layer, _message, _indentationValue, false);
		}

		//Called from LogImpl and RawLogImpl, taking _formatted accordingly
		void LogRawLogImpl(LOGGER_CHANNEL _channel, LOGGER_LAYER _layer, const std::string& _message, std::int32_t _indentationValue, bool _formatted) const;
	};

}