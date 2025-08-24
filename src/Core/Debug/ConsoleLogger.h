#pragma once
#include "ILogger.h"

namespace NK
{

	class ConsoleLogger final : public ILogger
	{
	public:
		explicit ConsoleLogger(const LoggerConfig& _config) : ILogger(_config) {}

		//Logs _message from _layer to the _channel channel (if the channel is enabled for the specified layer)
		//Newline characters are not included by this function, they should be contained within _message
		virtual void Log(LOGGER_CHANNEL _channel, LOGGER_LAYER _layer, const std::string& _message) const override;

		//Same as Log(), but doesn't include any formatting
		virtual void RawLog(LOGGER_CHANNEL _channel, LOGGER_LAYER _layer, const std::string& _message) const override;

	private:
		void LogImpl(LOGGER_CHANNEL _channel, LOGGER_LAYER _layer, const std::string& _message, bool _formatted) const;
	};

}