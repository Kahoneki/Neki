#pragma once

#include "TextureCompressor.h"

#include <thread>


namespace NK
{

	void TextureCompressor::BlockCompress(const std::string& _inputFilepath, const bool _srgb, const std::string& _outputFilepath, const DATA_FORMAT _outputFormat)
	{
		if (_outputFormat <= DATA_FORMAT::START_OF_BLOCK_COMPRESSION_FORMATS || _outputFormat >= DATA_FORMAT::END_OF_BLOCK_COMPRESSION_FORMATS)
		{
			throw std::invalid_argument("TextureCompressor::BlockCompress() - provided format (" + std::to_string(std::to_underlying(_outputFormat)) + ") is not a recognised block compression format as required");
		}

		m_context.enableCudaAcceleration(true); //attempt to enable
		nvtt::Surface srcTexture;
		if (!srcTexture.load(_inputFilepath.c_str())) //todo: look into hasAlpha parameter for transparency support
		{
			throw std::runtime_error("TextureCompressor::BlockCompress() - Failed to load source texture.");
		}

		nvtt::CompressionOptions compression{};
		compression.setFormat(GetNVTTFormat(_outputFormat));
		compression.setQuality(nvtt::Quality_Highest);

		nvtt::OutputOptions output;
		output.setFileName(_outputFilepath.c_str());
		output.setSrgbFlag(_srgb);
		if (!m_context.compress(srcTexture, 0, 0, compression, output))
		{
			throw std::runtime_error("TextureCompressor::BlockCompress() - Failed to compress texture.");
		}
	}



	nvtt::Format TextureCompressor::GetNVTTFormat(const DATA_FORMAT _format)
	{
	    switch (_format)
	    {
	    case DATA_FORMAT::BC1_RGB_UNORM:    	return nvtt::Format_BC1;
	    case DATA_FORMAT::BC1_RGB_SRGB:     	return nvtt::Format_BC1;
	    case DATA_FORMAT::BC3_RGBA_UNORM:   	return nvtt::Format_BC3;
	    case DATA_FORMAT::BC3_RGBA_SRGB:    	return nvtt::Format_BC3;
	    case DATA_FORMAT::BC4_R_UNORM:      	return nvtt::Format_BC4;
	    case DATA_FORMAT::BC4_R_SNORM:      	return nvtt::Format_BC4S;
	    case DATA_FORMAT::BC5_RG_UNORM:     	return nvtt::Format_BC5;
	    case DATA_FORMAT::BC5_RG_SNORM:     	return nvtt::Format_BC5S;
	    case DATA_FORMAT::BC7_RGBA_UNORM:   	return nvtt::Format_BC7;
	    case DATA_FORMAT::BC7_RGBA_SRGB:    	return nvtt::Format_BC7;
	    
	    default:
	    {
	        throw std::invalid_argument("TextureCompressor::GetNVTTFormat() default case reached. Format = " + std::to_string(std::to_underlying(_format)));
	    }
	    }
	}

}