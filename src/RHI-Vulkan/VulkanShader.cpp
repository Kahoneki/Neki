#include "VulkanShader.h"
#include <fstream>
#include <stdexcept>

namespace NK
{

	VulkanShader::VulkanShader(ILogger& _logger, const ShaderDesc& _desc)
	: IShader(_logger, _desc)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::SHADER, "Initialising VulkanShader\n");

		
		std::ifstream file(_desc.filepath + std::string(".spv"), std::ios::ate | std::ios::binary);
		if (!file.is_open())
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::SHADER, "Failed to open shader file at filepath: " + _desc.filepath + ".spv\n");
			throw std::runtime_error("");
		}
		const std::streamsize fileSize{ static_cast<std::streamsize>(file.tellg()) };
		m_bytecode.resize(fileSize);
		file.seekg(0);
		file.read(m_bytecode.data(), fileSize);
		file.close();


		m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::SHADER, "Successfully read shader file at filepath: " + _desc.filepath + ".spv\n");
		
		m_logger.Unindent();
	}
	
}