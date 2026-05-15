#include "GPUUploader.h"

#include <Core/Utils/ImageLoader.h>
#include <Core/Utils/NTCLoader.h>
#include <Core/Utils/NTCMatrixLayerConverter.h>
#include <RHI/IBuffer.h>
#include <RHI/IBufferView.h>
#include <RHI/ICommandBuffer.h>
#include <RHI/IQueue.h>
#include <RHI/ISemaphore.h>
#include <RHI/ITexture.h>
#include <RHI/ITextureView.h>
#include <RHI/RHIUtils.h>

#include <cstring>
#include <ranges>
#include <stdexcept>

#ifdef NEKI_VULKAN_SUPPORTED
	#include <RHI-Vulkan/VulkanDevice.h>
#endif


namespace NK
{

	GPUUploader::GPUUploader(ILogger& _logger, IDevice& _device, const GPUUploaderDesc& _desc)
	: m_logger(_logger), m_device(_device), m_stagingBufferSize(_desc.stagingBufferSize), m_queue(_desc.graphicsQueue)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::GPU_UPLOADER, "Initialising GPUUploader\n");

		if (m_queue->GetType() != COMMAND_TYPE::GRAPHICS)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::GPU_UPLOADER, "Provided _desc.graphicsQueue is not of type COMMAND_TYPE::GRAPHICS as required. Type = " + std::to_string(std::to_underlying(m_queue->GetType())) + "\n");
			throw std::runtime_error("");
		}


		BufferDesc stagingBufferDesc{};
		stagingBufferDesc.size = m_stagingBufferSize;
		stagingBufferDesc.type = MEMORY_TYPE::HOST;
		stagingBufferDesc.usage = BUFFER_USAGE_FLAGS::TRANSFER_SRC_BIT;
		m_stagingBuffer = m_device.CreateBuffer(stagingBufferDesc);
		m_stagingBufferMap = static_cast<unsigned char*>(m_stagingBuffer->GetMap());

		CommandPoolDesc commandPoolDesc{};
		commandPoolDesc.type = COMMAND_TYPE::GRAPHICS;
		m_commandPool = m_device.CreateCommandPool(commandPoolDesc);

		CommandBufferDesc commandBufferDesc{};
		commandBufferDesc.level = COMMAND_BUFFER_LEVEL::PRIMARY;
		m_commandBuffer = m_commandPool->AllocateCommandBuffer(commandBufferDesc);

		m_flushing = false;

		//Start recording commands
		m_commandBuffer->Reset();
		m_commandBuffer->Begin();

		m_commandBuffer->TransitionBarrier(m_stagingBuffer.get(), RESOURCE_STATE::UNDEFINED, RESOURCE_STATE::COPY_SOURCE);

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


		m_logger.Unindent();
	}



	void GPUUploader::EnqueueBufferDataUpload(const void* _data, IBuffer* _dstBuffer, RESOURCE_STATE _dstBufferInitialState)
	{
		if (m_flushing)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::GPU_UPLOADER, "EnqueueBufferDataUploaded() called while m_flushing was true. Did you forget to call Reset()?\n");
			throw std::runtime_error("");
		}
		
		const std::size_t unalignedOffset{ m_stagingBufferSubregions.empty() ? 0 : m_stagingBufferSubregions.back().offset + m_stagingBufferSubregions.back().size };
		constexpr std::size_t alignment{ 16 };
		const std::size_t alignedOffset{ (unalignedOffset + alignment - 1) & ~(alignment - 1) };
		
		BufferSubregion subregion{};
		subregion.offset = alignedOffset;
		subregion.size = _dstBuffer->GetSize();

		if (subregion.offset + subregion.size > m_stagingBufferSize)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::GPU_UPLOADER, "EnqueueBufferDataUpload() exceeded m_stagingBufferSize - calling Flush(true) and Reset().\n");
			Flush(true, nullptr, nullptr);
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



	void GPUUploader::EnqueueArrayTextureDataUpload(void* const* _data, ITexture* _dstTexture, RESOURCE_STATE _dstTextureInitialState)
	{
		if (m_flushing)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::GPU_UPLOADER, "EnqueueArrayTextureDataUploaded() called while m_flushing was true. Did you forget to call Reset()?\n");
			throw std::runtime_error("");
		}

		const TextureCopyMemoryLayout memLayout{ m_device.GetRequiredMemoryLayoutForTextureCopy(_dstTexture) };

		const std::size_t unalignedOffset{ m_stagingBufferSubregions.empty() ? 0 : m_stagingBufferSubregions.back().offset + m_stagingBufferSubregions.back().size };
		constexpr std::size_t alignment{ 16 };
		const std::size_t alignedOffset{ (unalignedOffset + alignment - 1) & ~(alignment - 1) };
		
		BufferSubregion subregion{};
		subregion.offset = alignedOffset;
		subregion.size = memLayout.totalBytes;

		if (subregion.offset + subregion.size > m_stagingBufferSize)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::GPU_UPLOADER, "EnqueueArrayTextureDataUpload() exceeded m_stagingBufferSize - flushing staging buffer with _waitIdle = true.\n");
			Flush(true, nullptr, nullptr);
			Reset();

			subregion.offset = 0;
		}

		//memcpy data one row at a time, advancing ptr by memLayout.rowPitch after each row
		unsigned char* dstPtr{ m_stagingBufferMap + subregion.offset };
		std::size_t numRows{};
		std::size_t numTextures{};
		glm::ivec3 copyDstExtent{};
		switch (_dstTexture->GetDimension())
		{
		case TEXTURE_DIMENSION::DIM_1:
		{
			numRows = 1;
			numTextures = _dstTexture->GetSize().y;
			copyDstExtent = glm::ivec3(_dstTexture->GetSize().x, 1, 1);
			break;
		}
		case TEXTURE_DIMENSION::DIM_2:
		{
			numRows = _dstTexture->GetSize().y;
			numTextures = _dstTexture->GetSize().z;
			copyDstExtent = glm::ivec3(_dstTexture->GetSize().x, _dstTexture->GetSize().y, 1);
			break;
		}
		case TEXTURE_DIMENSION::DIM_3:
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::GPU_UPLOADER, "EnqueueArrayTextureDataUpload() - Dimension of underlying texture elements in array texture is TEXTURE_DIMENSION::DIM_3 - this should not be possible and indicates an internal error with the engine itself. Please make a GitHub issue on the topic.\n");
			throw std::runtime_error("");
		}
		}

		//Loop through every texture in the array
		for (std::size_t texture{ 0 }; texture < numTextures; ++texture)
		{
			const unsigned char* currentTextureSrcPtr{ static_cast<const unsigned char*>(_data[texture]) };
			//Loop through every row in the texture
			for (std::size_t row{ 0 }; row < numRows; ++row)
			{
				memcpy(dstPtr, currentTextureSrcPtr, memLayout.rowPitch);
				currentTextureSrcPtr += memLayout.rowPitch;
				dstPtr += memLayout.rowPitch;
			}
		}
		m_stagingBufferSubregions.push_back(subregion);

		if (_dstTextureInitialState != RESOURCE_STATE::COPY_DEST)
		{
			m_commandBuffer->TransitionBarrier(_dstTexture, _dstTextureInitialState, RESOURCE_STATE::COPY_DEST);
		}

		m_commandBuffer->CopyBufferToTexture(m_stagingBuffer.get(), _dstTexture, subregion.offset, { 0, 0, 0 }, copyDstExtent, 0);
	}



	void GPUUploader::EnqueueTextureDataUpload(const void* _data, ITexture* _dstTexture, RESOURCE_STATE _dstTextureInitialState)
	{
	    if (m_flushing)
	    {
	        m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::GPU_UPLOADER, "EnqueueTextureDataUploaded() called while m_flushing was true. Did you forget to call Reset()?\n");
	        throw std::runtime_error("");
	    }

	    constexpr std::size_t TEXTURE_COPY_ALIGNMENT{ 512u };

		//Get total size of upload for staging buffer reservation
	    std::size_t totalUploadSize{ 0 };
	    glm::ivec3 mipSize{ _dstTexture->GetSize() };
	    for (std::uint32_t i{ 0 }; i < _dstTexture->GetMipLevels(); ++i)
	    {
	        std::size_t mipRowPitch{ 0 };
	        std::size_t mipHeight{ 0 };

	        if (RHIUtils::IsBlockCompressed(_dstTexture->GetFormat()))
	        {
	            mipRowPitch = ((mipSize.x + 3) / 4) * RHIUtils::GetBlockByteSize(_dstTexture->GetFormat());
	            mipHeight = (mipSize.y + 3) / 4;
	        }
	        else
	        {
	            mipRowPitch = mipSize.x * RHIUtils::GetFormatBytesPerPixel(_dstTexture->GetFormat());
	            mipHeight = mipSize.y;
	        }

	        const std::size_t mipBytes{ mipRowPitch * mipHeight * (_dstTexture->IsArrayTexture() ? (_dstTexture->GetDimension() == TEXTURE_DIMENSION::DIM_1 ? _dstTexture->GetSize().y : _dstTexture->GetSize().z) : 1) };
	        
	        //Align mip start
	        totalUploadSize = (totalUploadSize + TEXTURE_COPY_ALIGNMENT - 1) & ~(TEXTURE_COPY_ALIGNMENT - 1);
	        totalUploadSize += mipBytes;

    		//Just to be safe (final mip is 1x1)
	        mipSize.x = std::max(1, mipSize.x / 2);
	        mipSize.y = std::max(1, mipSize.y / 2);
	        mipSize.z = std::max(1, mipSize.z / 2);
	    }

	    //Calculate global offset into the staging buffer for subregion
	    const std::size_t unalignedOffset{ m_stagingBufferSubregions.empty() ? 0 : m_stagingBufferSubregions.back().offset + m_stagingBufferSubregions.back().size };
	    const std::size_t alignedOffset{ (unalignedOffset + TEXTURE_COPY_ALIGNMENT - 1) & ~(TEXTURE_COPY_ALIGNMENT - 1) };
	    
	    BufferSubregion subregion{};
	    subregion.offset = alignedOffset;
	    subregion.size = totalUploadSize;

	    if (subregion.offset + subregion.size > m_stagingBufferSize)
	    {
	        m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::GPU_UPLOADER, "EnqueueTextureDataUpload() exceeded m_stagingBufferSize - flushing staging buffer with _waitIdle = true.\n");
	        Flush(true, nullptr, nullptr);
	        Reset();
	        subregion.offset = 0;
	    }

	    //Main copy loop
	    const unsigned char* srcPtr{ static_cast<const unsigned char*>(_data) };
	    unsigned char* baseDstPtr{ m_stagingBufferMap + subregion.offset };
	    std::size_t currentBufferOffset = 0;
	    mipSize = _dstTexture->GetSize();

	    if (_dstTextureInitialState != RESOURCE_STATE::COPY_DEST)
	    {
	        m_commandBuffer->TransitionBarrier(_dstTexture, _dstTextureInitialState, RESOURCE_STATE::COPY_DEST);
	    }

	    for (std::uint32_t mip{ 0 }; mip < _dstTexture->GetMipLevels(); ++mip)
	    {
	        //Calculate layout for the current mip level
	        std::size_t sourceRowPitch = 0;
	        std::size_t numRows = 0;
	        if (RHIUtils::IsBlockCompressed(_dstTexture->GetFormat()))
	        {
	            sourceRowPitch = ((mipSize.x + 3) / 4) * RHIUtils::GetBlockByteSize(_dstTexture->GetFormat());
	            numRows = (mipSize.y + 3) / 4;
	        }
	        else
	        {
	            sourceRowPitch = mipSize.x * RHIUtils::GetFormatBytesPerPixel(_dstTexture->GetFormat());
	            numRows = mipSize.y;
	        }

	        //Calculate aligned offset into buffer for the current mip level
	        currentBufferOffset = (currentBufferOffset + TEXTURE_COPY_ALIGNMENT - 1) & ~(TEXTURE_COPY_ALIGNMENT - 1);
	        
	        //Determine layers
	        std::size_t numLayers = 1;
	        if (_dstTexture->IsArrayTexture())
	        {
	            numLayers = (_dstTexture->GetDimension() == TEXTURE_DIMENSION::DIM_1 ? _dstTexture->GetSize().y : _dstTexture->GetSize().z);
	        }
	        
	        //Issue copy for all layers for the current mip
	        for (std::size_t layer = 0; layer < numLayers; ++layer)
	        {
	            const std::size_t layerSize{ sourceRowPitch * numRows };
	            unsigned char* dst{ baseDstPtr + currentBufferOffset };
	            memcpy(dst, srcPtr, layerSize);
	            
	            //Enqueue the copy command
	            glm::ivec3 copyExtent{ mipSize.x, mipSize.y, 1 };
	            if (_dstTexture->GetDimension() == TEXTURE_DIMENSION::DIM_3) { copyExtent.z = mipSize.z; }
	            glm::ivec3 copyOffset{0, 0, 0};
	            if (_dstTexture->IsArrayTexture()) { copyOffset.z = layer; }
	            m_commandBuffer->CopyBufferToTexture(m_stagingBuffer.get(), _dstTexture, subregion.offset + currentBufferOffset, copyOffset, copyExtent, mip);

        		//Prepare for next layer
	            srcPtr += layerSize;
	            currentBufferOffset += layerSize;
	        }

	        //Prepare for next mip
	        mipSize.x = std::max(1, mipSize.x / 2);
	        mipSize.y = std::max(1, mipSize.y / 2);
	        mipSize.z = std::max(1, mipSize.z / 2);
	    }

	    m_stagingBufferSubregions.push_back(subregion);
	}

	UniquePtr<GPUMesh> GPUUploader::EnqueueMeshDataUpload(const CPUMeshData* _cpuMesh)
	{
		UniquePtr<GPUMesh> gpuMesh{ NK_NEW(GPUMesh) };
			
		//Vertex buffer
		BufferDesc vertexBufferDesc{};
		vertexBufferDesc.size = sizeof(Vertex) * _cpuMesh->vertices.size();
		vertexBufferDesc.type = MEMORY_TYPE::DEVICE;
		vertexBufferDesc.usage = BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | BUFFER_USAGE_FLAGS::VERTEX_BUFFER_BIT;
		gpuMesh->vertexBuffer = m_device.CreateBuffer(vertexBufferDesc);
		EnqueueBufferDataUpload(_cpuMesh->vertices.data(), gpuMesh->vertexBuffer.get(), RESOURCE_STATE::UNDEFINED);
		m_commandBuffer->TransitionBarrier(gpuMesh->vertexBuffer.get(), RESOURCE_STATE::COPY_DEST, RESOURCE_STATE::VERTEX_BUFFER);
		
		//Index buffer
		BufferDesc indexBufferDesc{};
		indexBufferDesc.size = sizeof(std::uint32_t) * _cpuMesh->indices.size();
		indexBufferDesc.type = MEMORY_TYPE::DEVICE;
		indexBufferDesc.usage = BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | BUFFER_USAGE_FLAGS::INDEX_BUFFER_BIT;
		gpuMesh->indexBuffer = m_device.CreateBuffer(indexBufferDesc);
		gpuMesh->indexCount = _cpuMesh->indices.size();
		EnqueueBufferDataUpload(_cpuMesh->indices.data(), gpuMesh->indexBuffer.get(), RESOURCE_STATE::UNDEFINED);
		m_commandBuffer->TransitionBarrier(gpuMesh->indexBuffer.get(), RESOURCE_STATE::COPY_DEST, RESOURCE_STATE::INDEX_BUFFER);
		
		return gpuMesh;
	}

		
		
	UniquePtr<GPUMaterial> GPUUploader::EnqueueMaterialDataUpload(const CPUMaterial* _cpuMaterial)
	{
		UniquePtr<GPUMaterial> gpuMaterial{ NK_NEW(GPUMaterial) };
		gpuMaterial->lightingModel = _cpuMaterial->pipeline;
			
		BufferDesc materialBufferDesc{};
		materialBufferDesc.type = MEMORY_TYPE::DEVICE;
		materialBufferDesc.usage = BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | BUFFER_USAGE_FLAGS::UNIFORM_BUFFER_BIT;

		if (gpuMaterial->lightingModel == LIGHTING_MODEL::BLINN_PHONG)
		{
			const BlinnPhongMaterial material{ std::get<BlinnPhongMaterial>(_cpuMaterial->shaderMaterialData) };
			materialBufferDesc.size = sizeof(BlinnPhongMaterial);
			gpuMaterial->materialBuffer = m_device.CreateBuffer(materialBufferDesc);
			EnqueueBufferDataUpload(&material, gpuMaterial->materialBuffer.get(), RESOURCE_STATE::UNDEFINED);
		}
		else
		{
			const PBRMaterial material{ std::get<PBRMaterial>(_cpuMaterial->shaderMaterialData) };
			materialBufferDesc.size = sizeof(PBRMaterial);
			gpuMaterial->materialBuffer = m_device.CreateBuffer(materialBufferDesc);
			EnqueueBufferDataUpload(&material, gpuMaterial->materialBuffer.get(), RESOURCE_STATE::UNDEFINED);
		}

		m_commandBuffer->TransitionBarrier(gpuMaterial->materialBuffer.get(), RESOURCE_STATE::COPY_DEST, RESOURCE_STATE::CONSTANT_BUFFER);

		BufferViewDesc materialBufferViewDesc{};
		materialBufferViewDesc.size = materialBufferDesc.size;
		materialBufferViewDesc.type = BUFFER_VIEW_TYPE::UNIFORM;
		materialBufferViewDesc.offset = 0;
		gpuMaterial->materialBufferView = m_device.CreateBufferView(gpuMaterial->materialBuffer.get(), materialBufferViewDesc);
		gpuMaterial->bufferIndex = gpuMaterial->materialBufferView->GetIndex();

		return gpuMaterial;
	}

	
	
	UniquePtr<GPUMaterial> GPUUploader::EnqueueMaterialDataUploadNTC(const CPUMaterialNTC* _cpuMaterial)
	{
	    UniquePtr<GPUMaterial> gpuMaterial{ NK_NEW(GPUMaterial) };
	    gpuMaterial->lightingModel = _cpuMaterial->pipeline;
	    gpuMaterial->isNTC = true;

	    Neural::NTCModel* ntcModel{ Neural::NTCLoader::LoadMaterial(_cpuMaterial->materialDataFilepath) };

	    BufferDesc materialBufferDesc{};
	    materialBufferDesc.type = MEMORY_TYPE::DEVICE;
	    materialBufferDesc.usage = BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | BUFFER_USAGE_FLAGS::UNIFORM_BUFFER_BIT;
	    
	    if (gpuMaterial->lightingModel == LIGHTING_MODEL::PHYSICALLY_BASED)
	    {
	        materialBufferDesc.size = sizeof(PBRMaterialNTC);
	        gpuMaterial->materialBuffer = m_device.CreateBuffer(materialBufferDesc);
	        PBRMaterialNTC pbrMat{ std::get<PBRMaterialNTC>(_cpuMaterial->shaderMaterialData) };
	        EnqueueBufferDataUpload(&pbrMat, gpuMaterial->materialBuffer.get(), RESOURCE_STATE::UNDEFINED);
	    }
	    else
	    {
	        materialBufferDesc.size = sizeof(BlinnPhongMaterialNTC);
	        gpuMaterial->materialBuffer = m_device.CreateBuffer(materialBufferDesc);
	        BlinnPhongMaterialNTC bpMat{ std::get<BlinnPhongMaterialNTC>(_cpuMaterial->shaderMaterialData) };
	        EnqueueBufferDataUpload(&bpMat, gpuMaterial->materialBuffer.get(), RESOURCE_STATE::UNDEFINED);
	    }

	    m_commandBuffer->TransitionBarrier(gpuMaterial->materialBuffer.get(), RESOURCE_STATE::COPY_DEST, RESOURCE_STATE::CONSTANT_BUFFER);

	    BufferViewDesc materialBufferViewDesc{};
	    materialBufferViewDesc.size = materialBufferDesc.size;
	    materialBufferViewDesc.type = BUFFER_VIEW_TYPE::UNIFORM;
	    materialBufferViewDesc.offset = 0;
	    gpuMaterial->materialBufferView = m_device.CreateBufferView(gpuMaterial->materialBuffer.get(), materialBufferViewDesc);
	    gpuMaterial->bufferIndex = gpuMaterial->materialBufferView->GetIndex();

	    //Populate NTC metadata
	    gpuMaterial->g0Channels = ntcModel->header.g0Channels;
	    gpuMaterial->g1Channels = ntcModel->header.g1Channels;
	    gpuMaterial->g0QuantLevels = ntcModel->header.g0QuantLevels;
	    gpuMaterial->g1QuantLevels = ntcModel->header.g1QuantLevels;
	    gpuMaterial->g0Resolution = ntcModel->header.baseImageRes;
	    gpuMaterial->imageResolution = ntcModel->header.baseImageRes;
	    gpuMaterial->numOctaves = ntcModel->header.numOctaves;
	    gpuMaterial->tileSize = ntcModel->header.tileSize;
	    gpuMaterial->numLayers = ntcModel->header.numLinearLayers;
	    gpuMaterial->hiddenNeurons = ntcModel->header.hiddenNeurons;

		//Upload G0 buffer (combining all 4 feature levels into a single buffer)
	    std::size_t g0TotalSize{ 0 };
	    for(std::size_t i{ 0 }; i < 4; ++i)
	    {
		    g0TotalSize += ntcModel->featureLevels[i].g0.numElementsPacked;
	    }
	    BufferDesc g0Desc{};
	    g0Desc.size = g0TotalSize;
	    g0Desc.type = MEMORY_TYPE::DEVICE;
	    g0Desc.usage = BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | BUFFER_USAGE_FLAGS::STORAGE_BUFFER_READ_ONLY_BIT;
	    gpuMaterial->g0Buffer = m_device.CreateBuffer(g0Desc);
	    std::vector<unsigned char> g0Data(g0TotalSize);
	    std::size_t g0Offset{ 0 };
	    for(std::size_t i{ 0 }; i < 4; ++i)
	    {
	        std::memcpy(g0Data.data() + g0Offset, ntcModel->featureLevels[i].g0.data, ntcModel->featureLevels[i].g0.numElementsPacked);
	        g0Offset += ntcModel->featureLevels[i].g0.numElementsPacked;
	    }
	    EnqueueBufferDataUpload(g0Data.data(), gpuMaterial->g0Buffer.get(), RESOURCE_STATE::UNDEFINED);
	    m_commandBuffer->TransitionBarrier(gpuMaterial->g0Buffer.get(), RESOURCE_STATE::COPY_DEST, RESOURCE_STATE::SHADER_RESOURCE);
	    BufferViewDesc g0ViewDesc{};
	    g0ViewDesc.size = g0TotalSize;
	    g0ViewDesc.type = BUFFER_VIEW_TYPE::STORAGE_READ_ONLY;
	    g0ViewDesc.offset = 0;
	    g0ViewDesc.stride = 0; //ByteAddressBuffer
	    gpuMaterial->g0BufferView = m_device.CreateBufferView(gpuMaterial->g0Buffer.get(), g0ViewDesc);

	    //Upload G1 buffer (combining all 4 feature levels into a single buffer)
	    std::size_t g1TotalSize{ 0 };
	    for(std::size_t i{ 0 }; i < 4; ++i)
	    {
		    g1TotalSize += ntcModel->featureLevels[i].g1.numElementsPacked;
	    }
	    BufferDesc g1Desc{};
	    g1Desc.size = g1TotalSize;
	    g1Desc.type = MEMORY_TYPE::DEVICE;
	    g1Desc.usage = BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | BUFFER_USAGE_FLAGS::STORAGE_BUFFER_READ_ONLY_BIT;
	    gpuMaterial->g1Buffer = m_device.CreateBuffer(g1Desc);
	    std::vector<unsigned char> g1Data(g1TotalSize);
	    std::size_t g1Offset{ 0 };
	    for(std::size_t i = 0; i < 4; ++i)
	    {
	        std::memcpy(g1Data.data() + g1Offset, ntcModel->featureLevels[i].g1.data, ntcModel->featureLevels[i].g1.numElementsPacked);
	        g1Offset += ntcModel->featureLevels[i].g1.numElementsPacked;
	    }
	    EnqueueBufferDataUpload(g1Data.data(), gpuMaterial->g1Buffer.get(), RESOURCE_STATE::UNDEFINED);
	    m_commandBuffer->TransitionBarrier(gpuMaterial->g1Buffer.get(), RESOURCE_STATE::COPY_DEST, RESOURCE_STATE::SHADER_RESOURCE);
	    BufferViewDesc g1ViewDesc{};
	    g1ViewDesc.size = g1TotalSize;
	    g1ViewDesc.type = BUFFER_VIEW_TYPE::STORAGE_READ_ONLY;
	    g1ViewDesc.offset = 0;
	    g1ViewDesc.stride = 0;
	    gpuMaterial->g1BufferView = m_device.CreateBufferView(gpuMaterial->g1Buffer.get(), g1ViewDesc);

	    //Upload MLP Buffer (converting for cooperative matrix)
	    std::vector<Neural::ConvertedLayer> convertedLayers;
	    std::size_t mlpTotalSize{ 0 };
	    if (VkDevice vkDevice{ dynamic_cast<VulkanDevice&>(m_device).GetDevice() })
	    {
		    for (const Neural::NTCLinearLayer& layer : ntcModel->mlp)
		    {
		    	Neural::ConvertedLayer convertedLayer{ Neural::NTCMatrixLayerConverter::ConvertLayer(vkDevice, layer.weights.data(), layer.weights.size() * sizeof(std::uint16_t), layer.biases.data(), layer.biases.size() * sizeof(std::uint16_t), layer.outFeatures, layer.inFeatures) };
		    	convertedLayers.push_back(convertedLayer);
		    	mlpTotalSize = (mlpTotalSize + 15) & ~15;
		    	mlpTotalSize += convertedLayer.weightData.size();
		    	mlpTotalSize = (mlpTotalSize + 15) & ~15;
		    	mlpTotalSize += convertedLayer.biasData.size();
		    }
	    }
	    else
	    {
		    throw std::runtime_error("Neural materials currently require Vulkan for cooperative matrix support.");
	    }
	    BufferDesc mlpDesc{};
	    mlpDesc.size = mlpTotalSize;
	    mlpDesc.type = MEMORY_TYPE::DEVICE;
	    mlpDesc.usage = BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | BUFFER_USAGE_FLAGS::STORAGE_BUFFER_READ_ONLY_BIT;
	    gpuMaterial->mlpBuffer = m_device.CreateBuffer(mlpDesc);
	    std::vector<unsigned char> mlpData(mlpTotalSize, 0);
	    std::size_t currentMlpOffset{ 0 };
	    auto appendData{ [&](const std::vector<std::uint8_t>& data, std::uint32_t& outOffset)
		{
	        currentMlpOffset = (currentMlpOffset + 15) & ~15; //16-byte align
	        outOffset = currentMlpOffset;
	        std::memcpy(mlpData.data() + currentMlpOffset, data.data(), data.size());
	        currentMlpOffset += data.size();
	    } };
	    if (convertedLayers.size() > 0)
	    {
	        appendData(convertedLayers[0].weightData, gpuMaterial->layer0_W_offset);
	        appendData(convertedLayers[0].biasData, gpuMaterial->layer0_B_offset);
	    }
	    if (convertedLayers.size() > 1)
	    {
	        appendData(convertedLayers[1].weightData, gpuMaterial->layer1_W_offset);
	        appendData(convertedLayers[1].biasData, gpuMaterial->layer1_B_offset);
	    }
	    if (convertedLayers.size() > 2)
	    {
	        appendData(convertedLayers[2].weightData, gpuMaterial->layer2_W_offset);
	        appendData(convertedLayers[2].biasData, gpuMaterial->layer2_B_offset);
	    }

	    EnqueueBufferDataUpload(mlpData.data(), gpuMaterial->mlpBuffer.get(), RESOURCE_STATE::UNDEFINED);
	    m_commandBuffer->TransitionBarrier(gpuMaterial->mlpBuffer.get(), RESOURCE_STATE::COPY_DEST, RESOURCE_STATE::SHADER_RESOURCE);

	    BufferViewDesc mlpViewDesc{};
	    mlpViewDesc.size = mlpTotalSize;
	    mlpViewDesc.type = BUFFER_VIEW_TYPE::STORAGE_READ_ONLY;
	    mlpViewDesc.offset = 0;
	    mlpViewDesc.stride = 0;
	    gpuMaterial->mlpBufferView = m_device.CreateBufferView(gpuMaterial->mlpBuffer.get(), mlpViewDesc);

	    //Free cpu-side ntc model (todo: do we actually want to do this? (streaming purposes?))
	    Neural::NTCLoader::FreeMaterial(ntcModel);

	    return gpuMaterial;
	}


	
	UniquePtr<GPUTexture> GPUUploader::EnqueueTextureDataUpload(const ImageData* _imgData)
	{
		UniquePtr<GPUTexture> gpuTex{ NK_NEW(GPUTexture) };
		
		//Texture
		gpuTex->texture = m_device.CreateTexture(_imgData->desc);
		EnqueueTextureDataUpload(_imgData->data, gpuTex->texture.get(), RESOURCE_STATE::UNDEFINED);
		m_commandBuffer->TransitionBarrier(gpuTex->texture.get(), RESOURCE_STATE::COPY_DEST, RESOURCE_STATE::SHADER_RESOURCE);
		
		//View
		TextureViewDesc viewDesc{};
		viewDesc.type = TEXTURE_VIEW_TYPE::SHADER_READ_ONLY;
		viewDesc.dimension = TEXTURE_VIEW_DIMENSION::DIM_2;
		viewDesc.format = gpuTex->texture->GetFormat();
		gpuTex->view = m_device.CreateShaderResourceTextureView(gpuTex->texture.get(), viewDesc);
		
		return gpuTex;
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



	void GPUUploader::Flush(bool _waitIdle, IFence* _signalFence, ISemaphore* _signalSemaphore)
	{
		if (!_waitIdle && !_signalFence && !_signalSemaphore)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::GPU_UPLOADER, "In Flush() - _waitIdle = false, _signalFence = nullptr, _signalSemaphore - you have no way of knowing when the flush has finished - this is considered bad practice and is almost certainly a mistake.\n");
		}
		
		if (m_flushing)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::GPU_UPLOADER, "Attempted to call Flush() while GPUUploader was already flushing.\n");
			throw std::runtime_error("");
		}
		
		m_commandBuffer->End();
		
		m_flushing = true;
		m_queue->Submit(m_commandBuffer.get(), nullptr, _signalSemaphore, _signalFence);

		if (_waitIdle)
		{
			m_queue->WaitIdle();
		}
	}

}
