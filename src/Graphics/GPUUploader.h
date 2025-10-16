#pragma once

#include <Core/Memory/Allocation.h>
#include <Core/Utils/ModelLoader.h>
#include <RHI/ITextureView.h>
#include <Types/NekiTypes.h>

#include <cstddef>
#include <vector>


namespace NK
{
	
	class ILogger;
	class IDevice;
	class IBuffer;
	class IBufferView;
	class ICommandPool;
	class ICommandBuffer;
	class IQueue;
	class ITexture;
	class IFence;
	typedef std::uint32_t RESOURCE_INDEX; //Todo: this is a duplicate definition
	
	struct BufferSubregion
	{
		std::size_t offset;
		std::size_t size;
	};

	struct GPUUploaderDesc
	{
		//Size of the staging buffer in bytes - if the staging buffer is filled without flushing, a blocking flush will be automatically issued
		std::size_t stagingBufferSize;

		//A non-owning copy of a transfer queue. The GPUUploader will use it when flushing commands, synchronisation should be handled externally by the caller
		//When flushing, either set the _waitIdle parameter to true or use the returned fence to know when the GPUUploader is done using the queue
		//Note: there is no logical impact (beyond performance) in submitting commands to the transfer queue while the GPUUploader is using it
		IQueue* transferQueue;
	};

	
	struct GPUMesh
	{
		UniquePtr<IBuffer> vertexBuffer;
		UniquePtr<IBuffer> indexBuffer;
		std::uint32_t indexCount;
		std::size_t materialIndex; //Index into parent GPUModel's materials vector
	};

	struct GPUMaterial
	{
		LIGHTING_MODEL lightingModel;
		RESOURCE_INDEX bufferIndex;

		//This material will hence be valid for as long as its in scope
		UniquePtr<IBuffer> materialBuffer;
		UniquePtr<IBufferView> materialBufferView;
		std::vector<UniquePtr<ITexture>> textures;
		std::vector<UniquePtr<ITextureView>> textureViews;
	};
	
	struct GPUModel
	{
		std::vector<UniquePtr<GPUMesh>> meshes;
		std::vector<UniquePtr<GPUMaterial>> materials;
	};
	
	
	class GPUUploader final
	{
	public:
		GPUUploader(ILogger& _logger, IDevice& _device, const GPUUploaderDesc& _desc);
		~GPUUploader();

		//_dstBufferInitialState is the state of _dstBuffer
		//Regardless of the initial state, the state after the buffer data has been uploaded will be RESOURCE_STATE::COPY_DEST
		//The number of bytes in _data is expected to exactly match the size of the destination buffer
		void EnqueueBufferDataUpload(const void* _data, IBuffer* _dstBuffer, RESOURCE_STATE _dstBufferInitialState);

		//_dstTextureInitialState is the state of _dstTexture
		//Regardless of the initial state, the state after the texture data has been uploaded will be RESOURCE_STATE::COPY_DEST
		//_data is a const array of void pointers, each item in the array should be the data for a layer in the array texture
		//The number of bytes in each element of _data is expected to exactly match the size of the destination texture's layers
		//The number of elements in _data is expected to exactly match the number of array layers in the destination texture
		void EnqueueArrayTextureDataUpload(void* const* _data, ITexture* _dstTexture, RESOURCE_STATE _dstTextureInitialState);
		
		//_dstTextureInitialState is the state of _dstTexture
		//Regardless of the initial state, the state after the texture data has been uploaded will be RESOURCE_STATE::COPY_DEST
		//The number of bytes in _data is expected to exactly match the size of the destination texture
		void EnqueueTextureDataUpload(const void* _data, ITexture* _dstTexture, RESOURCE_STATE _dstTextureInitialState);

		//_cpuModel should be populated from ModelLoader::LoadModel()
		[[nodiscard]] UniquePtr<GPUModel> EnqueueModelDataUpload(const CPUModel* _cpuModel);

		//If _waitIdle = true, the calling thread will be blocked until the flush is complete and the returned fence will already be signalled
		//
		//If _waitIdle = false, the calling thread will not be blocked, instead a fence is returned that will be signalled when the flush is complete
		//
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
		//Gets cleared when Reset() is called
		std::vector<BufferSubregion> m_stagingBufferSubregions;
		
		UniquePtr<ICommandPool> m_commandPool;
		UniquePtr<ICommandBuffer> m_commandBuffer;
		
		IQueue* m_queue;

		bool m_flushing;
	};
	
}