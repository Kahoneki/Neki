#pragma once
#include <RHI/IBuffer.h>
#include <RHI/ITexture.h>
#include <RHI/ICommandBuffer.h>

namespace NK
{
	//--------------//
	//----UNUSED----//
	//--------------//
	
	//Utility class for uploading host-side staging buffers to device-side buffers
	class GPUUploader final
	{
	public:
		GPUUploader(ILogger& _logger);
		~GPUUploader() = default;

		//Todo: add subresource mapping ranges
		//Todo: add optional invalidate-buffer flag

		//Upload a _hostBuffer populated with data to a _deviceBuffer
		void UploadBuffer(IBuffer* _hostBuffer, IBuffer* _deviceBuffer, ICommandBuffer* _commandBuffer);

		//Upload a _hostTexture populated with data to a _deviceTexture
		void UploadTexture(ITexture* _hostTexture, ITexture* _deviceTexture, ICommandBuffer* _commandBuffer);


	private:
		//Dependency injections
		ILogger& m_logger;
	};

	//--------------//
	//----UNUSED----//
	//--------------//
}