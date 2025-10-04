#pragma once
#include <cstddef>
#include <vector>

#include "Core/Memory/Allocation.h"
#include "Types/ResourceStates.h"

namespace NK
{
	class ILogger;
	class IDevice;
	class IBuffer;
	class ICommandPool;
	class ICommandBuffer;
	class IQueue;
	class ITexture;
	class IFence;
	
	struct BufferSubregion
	{
		std::size_t offset;
		std::size_t size;
	};
	
	class GPUUploader final
	{
	public:
		GPUUploader(ILogger& _logger, IDevice& _device, std::size_t _stagingBufferSize);
		~GPUUploader();

		//_dstBufferInitialState is the state of _dstBuffer
		//Regardless of the initial state, the state after the buffer data has been uploaded will be RESOURCE_STATE::COPY_DEST
		void EnqueueBufferDataUpload(std::size_t _numBytes, const void* _data, IBuffer* _dstBuffer, RESOURCE_STATE _dstBufferInitialState);
		
		//_dstTextureInitialState is the state of _dstTexture
		//Regardless of the initial state, the state after the texture data has been uploaded will be RESOURCE_STATE::COPY_DEST
		void EnqueueTextureDataUpload(std::size_t _numBytes, const void* _data, ITexture* _dstTexture, RESOURCE_STATE _dstTextureInitialState);

		//If _waitIdle = true, the calling thread will be blocked until the flush is complete and nullptr is returned
		//
		//If _waitIdle = false, the calling thread will not be blocked, instead a fence is returned that will be signalled when the flush is complete
		//Once the flush has been complete, Reset() should be called.
		UniquePtr<IFence> Flush(bool _waitIdle);

		void Reset();
		

	private:
		//Dependency injections
		ILogger& m_logger;
		IDevice& m_device;

		const std::size_t m_stagingBufferSize;
		UniquePtr<IBuffer> m_stagingBuffer;
		unsigned char* m_stagingBufferMap;
		//There is one BufferSubregion for every resource in m_stagingBuffer
		//Subregions are tightly packed so m_stagingBufferSubregions.back().offset + m_stagingBufferSubregions.back().size will be the offset for new resources
		//Gets cleared when Flush() is called
		std::vector<BufferSubregion> m_stagingBufferSubregions;
		
		UniquePtr<ICommandPool> m_commandPool;
		UniquePtr<ICommandBuffer> m_commandBuffer;
		
		UniquePtr<IQueue> m_queue;

		bool m_flushing;
	};
}