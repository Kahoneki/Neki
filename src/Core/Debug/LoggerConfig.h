#pragma once

#include <Types/NekiTypes.h>

#include <unordered_map>


namespace NK
{


	class LoggerConfig final
	{
	public:
		explicit LoggerConfig(LOGGER_TYPE _type, bool _enableAll = false);
		~LoggerConfig() = default;

		//Set the default logging channels for all layers that have not been explicitly set
		void SetDefaultChannelBitfield(LOGGER_CHANNEL _channels);

		//Set the logging channels for a specific layer
		void SetLayerChannelBitfield(LOGGER_LAYER _layer, LOGGER_CHANNEL _channels);

		//Get the logging channels for a specific layer
		[[nodiscard]] LOGGER_CHANNEL GetChannelBitfieldForLayer(LOGGER_LAYER _layer) const;

		const LOGGER_TYPE type;


	private:
		LOGGER_CHANNEL m_defaultChannelBitfield;
		std::unordered_map<LOGGER_LAYER, LOGGER_CHANNEL> m_layerChannelBitfield;
	};
}