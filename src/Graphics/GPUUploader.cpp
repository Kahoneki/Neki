#include "GPUUploader.h"

#include <cstring>
#include <RHI/IBuffer.h>
#include <RHI/ICommandBuffer.h>
#include <RHI/IQueue.h>
#include <RHI/ITexture.h>

namespace NK
{

	GPUUploader::GPUUploader(ILogger& _logger, IDevice& _device, const GPUUploaderDesc& _desc)
	: m_logger(_logger), m_device(_device), m_stagingBufferSize(_desc.stagingBufferSize), m_queue(_desc.transferQueue)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::GPU_UPLOADER, "Initialising GPUUploader\n");

		if (m_queue->GetType() != COMMAND_POOL_TYPE::TRANSFER)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::GPU_UPLOADER, "Provided _desc.transferQueue is not of type COMMAND_POOL_TYPE::TRANSFER as required. Type = " + std::to_string(std::to_underlying(m_queue->GetType())) + "\n");
			throw std::runtime_error("");
		}


		BufferDesc stagingBufferDesc{};
		stagingBufferDesc.size = m_stagingBufferSize;
		stagingBufferDesc.type = MEMORY_TYPE::HOST;
		stagingBufferDesc.usage = BUFFER_USAGE_FLAGS::TRANSFER_SRC_BIT;
		m_stagingBuffer = m_device.CreateBuffer(stagingBufferDesc);
		m_stagingBufferMap = static_cast<unsigned char*>(m_stagingBuffer->Map());

		CommandPoolDesc commandPoolDesc{};
		commandPoolDesc.type = COMMAND_POOL_TYPE::TRANSFER;
		m_commandPool = m_device.CreateCommandPool(commandPoolDesc);

		CommandBufferDesc commandBufferDesc{};
		commandBufferDesc.level = COMMAND_BUFFER_LEVEL::PRIMARY;
		m_commandBuffer = m_commandPool->AllocateCommandBuffer(commandBufferDesc);

		m_flushing = false;

		//Start recording commands
		m_commandBuffer->Begin();


		m_logger.Unindent();
	}



	GPUUploader::~GPUUploader()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::GPU_UPLOADER, "Shutting Down GPUUploader\n");

		
		if (m_flushing)
		{
			m_queue->WaitIdle();
		}

		if (m_stagingBufferMap != nullptr)
		{
			m_stagingBuffer->Unmap();
			m_stagingBufferMap = nullptr;
			m_logger.IndentLog(LOGGER_CHANNEL::SUCCESS, LOGGER_LAYER::GPU_UPLOADER, "Staging Buffer Unmapped\n");
		}


		m_logger.Unindent();
	}



	void GPUUploader::EnqueueBufferDataUpload(std::size_t _numBytes, const void* _data, IBuffer* _dstBuffer, RESOURCE_STATE _dstBufferInitialState)
	{
		if (m_flushing)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::GPU_UPLOADER, "EnqueueBufferDataUploaded() called while m_flushing was true. Did you forget to call Reset()?\n");
			throw std::runtime_error("");
		}
		
		BufferSubregion subregion{};
		subregion.offset = (m_stagingBufferSubregions.empty() ? 0 : m_stagingBufferSubregions.back().offset + m_stagingBufferSubregions.back().size);
		subregion.size = _numBytes;

		if (subregion.offset + subregion.size > m_stagingBufferSize)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::GPU_UPLOADER, "EnqueueBufferDataUpload() exceeded m_stagingBufferSize - calling Flush(true) and Reset().\n");
			Flush(true);
			Reset();

			subregion.offset = 0;
		}
		
		memcpy(m_stagingBufferMap + subregion.offset, _data, subregion.size);
		m_stagingBufferSubregions.push_back(subregion);

		if (_dstBufferInitialState != RESOURCE_STATE::COPY_DEST)
		{
			m_commandBuffer->TransitionBarrier(_dstBuffer, _dstBufferInitialState, RESOURCE_STATE::COPY_DEST);
		}

		m_commandBuffer->CopyBufferToBuffer(m_stagingBuffer.get(), _dstBuffer, subregion.offset, 0, _dstBuffer->GetSize());
	}



	void GPUUploader::EnqueueTextureDataUpload(std::size_t _numBytes, const void* _data, ITexture* _dstTexture, RESOURCE_STATE _dstTextureInitialState)
	{
		if (m_flushing)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::GPU_UPLOADER, "EnqueueTextureDataUploaded() called while m_flushing was true. Did you forget to call Reset()?\n");
			throw std::runtime_error("");
		}
		
		BufferSubregion subregion{};
		subregion.offset = (m_stagingBufferSubregions.empty() ? 0 : m_stagingBufferSubregions.back().offset + m_stagingBufferSubregions.back().size);
		subregion.size = _numBytes;

		if (subregion.offset + subregion.size > m_stagingBufferSize)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::GPU_UPLOADER, "EnqueueTextureDataUpload() exceeded m_stagingBufferSize - flushing staging buffer with _waitIdle = true.\n");
			Flush(true);
			Reset();

			subregion.offset = 0;
		}
		
		memcpy(m_stagingBufferMap + subregion.offset, _data, subregion.size);
		m_stagingBufferSubregions.push_back(subregion);

		if (_dstTextureInitialState != RESOURCE_STATE::COPY_DEST)
		{
			m_commandBuffer->TransitionBarrier(_dstTexture, _dstTextureInitialState, RESOURCE_STATE::COPY_DEST);
		}

		m_commandBuffer->CopyBufferToTexture(m_stagingBuffer.get(), _dstTexture, subregion.offset, { 0, 0, 0 }, _dstTexture->GetSize());
	}



	void GPUUploader::Reset()
	{
		if (!m_flushing)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::GPU_UPLOADER, "Attempted to call Reset() while m_flushing was false (did you call Reset() twice?).\n");
			throw std::runtime_error("");
		}
		
		m_stagingBufferSubregions.clear();
		m_flushing = false;
		m_commandBuffer->Reset();
		m_commandBuffer->Begin();
	}



	UniquePtr<IFence> GPUUploader::Flush(bool _waitIdle)
	{
		if (m_flushing)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::GPU_UPLOADER, "Attempted to call Flush() while GPUUploader was already flushing.\n");
			throw std::runtime_error("");
		}
		
		m_commandBuffer->End();

		FenceDesc fenceDesc{};
		fenceDesc.initiallySignaled = false;
		UniquePtr<IFence> fence{ m_device.CreateFence(fenceDesc) };
		
		m_queue->Submit(m_commandBuffer.get(), nullptr, nullptr, fence.get());
		m_flushing = true;

		if (_waitIdle)
		{
			m_queue->WaitIdle();
			return nullptr;
		}
		else
		{
			return std::move(fence);
		}
	}

}