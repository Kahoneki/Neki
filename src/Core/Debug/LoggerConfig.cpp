#include "LoggerConfig.h"

namespace NK
{


	LoggerConfig::LoggerConfig(bool _enableAll)
	{
		if (_enableAll) { m_defaultChannelBitfield = LOGGER_CHANNEL::INFO | LOGGER_CHANNEL::HEADING | LOGGER_CHANNEL::WARNING | LOGGER_CHANNEL::ERROR | LOGGER_CHANNEL::SUCCESS; }
		else { m_defaultChannelBitfield = LOGGER_CHANNEL::NONE; }
	}



	void LoggerConfig::SetDefaultChannelBitfield(LOGGER_CHANNEL _channels)
	{
		m_defaultChannelBitfield = _channels;
	}



	void LoggerConfig::SetLayerChannelBitfield(LOGGER_LAYER _layer, LOGGER_CHANNEL _channels)
	{
		m_layerChannelBitfield[_layer] = _channels;
	}



	LOGGER_CHANNEL LoggerConfig::GetChannelBitfieldForLayer(LOGGER_LAYER _layer) const
	{
		//If a specific channel bitfield has been set for this layer, return it. Otherwise, just return the default
		const std::unordered_map<LOGGER_LAYER, LOGGER_CHANNEL>::const_iterator it{ m_layerChannelBitfield.find(_layer) };
		if (it != m_layerChannelBitfield.end())
		{
			return it->second;
		}
		return m_defaultChannelBitfield;
	}



}