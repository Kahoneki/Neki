#include "GPUUploader.h"
#include <Core/Utils/EnumUtils.h>
#include <stdexcept>

namespace NK
{
	
	GPUUploader::GPUUploader(ILogger& _logger)
		: m_logger(_logger)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::GPU_UPLOADER, "Initialising GPU Uploader\n");
		m_logger.Unindent();
	}


	void GPUUploader::UploadBuffer(IBuffer* _hostBuffer, IBuffer* _deviceBuffer, ICommandBuffer* _commandBuffer)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::GPU_UPLOADER, "Uploading host buffer to device buffer");
		

		//Input validation

		//Ensure host buffer is actually a host buffer
		if (_hostBuffer->GetMemoryType() != NK::MEMORY_TYPE::HOST)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::GPU_UPLOADER, "_hostBuffer wasn't created with NK::MEMORY_TYPE::HOST\n");
			throw std::runtime_error("");
		}

		//Ensure device buffer is actually a device buffer
		if (_hostBuffer->GetMemoryType() != NK::MEMORY_TYPE::DEVICE)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::GPU_UPLOADER, "_deviceBuffer wasn't created with NK::MEMORY_TYPE::DEVICE\n");
			throw std::runtime_error("");
		}

		//Ensure host buffer is transfer src capable
		if (!EnumUtils::Contains(_hostBuffer->GetUsageFlags(), NK::BUFFER_USAGE_FLAGS::TRANSFER_SRC_BIT))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::GPU_UPLOADER, "_hostBuffer wasn't created with NK::BUFFER_USAGE_FLAGS::TRANSFER_SRC_BIT\n");
			throw std::runtime_error("");
		}

		//Ensure device buffer is transfer dst capable
		if (!EnumUtils::Contains(_deviceBuffer->GetUsageFlags(), NK::BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT))
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::GPU_UPLOADER, "_deviceBuffer wasn't created with NK::BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT\n");
			throw std::runtime_error("");
		}


		//Record command buffer for copy



		m_logger.Unindent();
	}



	void GPUUploader::UploadTexture(ITexture* _hostTexture, ITexture* _deviceTexture, ICommandBuffer* _commandBuffer)
	{

	}

}