#pragma once

#include <Types/NekiTypes.h>

#include <string>
#include <nvtt/nvtt.h>


namespace NK
{

	class TextureCompressor
	{
	public:
		//Takes the file at _inputFilepath and compresses it to the _outputFormat provided (must be BC_ format), saving the result to a .dds at _outputFilepath
		static void BlockCompress(const std::string& _inputFilepath, const bool _srgb, const std::string& _outputFilepath, const DATA_FORMAT _outputFormat);


	private:
		[[nodiscard]] static nvtt::Format GetNVTTFormat(const DATA_FORMAT _format);

		static nvtt::Context m_context;
	};
	
}