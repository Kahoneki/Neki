#include "GPUUploader.h"

#include <Core/Utils/ImageLoader.h>
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



	UniquePtr<GPUModel> GPUUploader::EnqueueModelDataUpload(const CPUModel* _cpuModel)
	{
		UniquePtr<GPUModel> gpuModel{ NK_NEW(GPUModel) };
		
		//Load the mesh data
		for (const CPUMesh& cpuMesh : _cpuModel->meshes)
		{
			UniquePtr<GPUMesh> gpuMesh{ NK_NEW(GPUMesh) };
			
			//Vertex buffer
			BufferDesc vertexBufferDesc{};
			vertexBufferDesc.size = sizeof(ModelVertex) * cpuMesh.vertices.size();
			vertexBufferDesc.type = MEMORY_TYPE::DEVICE;
			vertexBufferDesc.usage = BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | BUFFER_USAGE_FLAGS::VERTEX_BUFFER_BIT;
			gpuMesh->vertexBuffer = m_device.CreateBuffer(vertexBufferDesc);
			EnqueueBufferDataUpload(cpuMesh.vertices.data(), gpuMesh->vertexBuffer.get(), RESOURCE_STATE::UNDEFINED);
			m_commandBuffer->TransitionBarrier(gpuMesh->vertexBuffer.get(), RESOURCE_STATE::COPY_DEST, RESOURCE_STATE::VERTEX_BUFFER);

			//Index buffer
			BufferDesc indexBufferDesc{};
			indexBufferDesc.size = sizeof(std::uint32_t) * cpuMesh.indices.size();
			indexBufferDesc.type = MEMORY_TYPE::DEVICE;
			indexBufferDesc.usage = BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | BUFFER_USAGE_FLAGS::INDEX_BUFFER_BIT;
			gpuMesh->indexBuffer = m_device.CreateBuffer(indexBufferDesc);
			gpuMesh->indexCount = cpuMesh.indices.size();
			EnqueueBufferDataUpload(cpuMesh.indices.data(), gpuMesh->indexBuffer.get(), RESOURCE_STATE::UNDEFINED);
			m_commandBuffer->TransitionBarrier(gpuMesh->indexBuffer.get(), RESOURCE_STATE::COPY_DEST, RESOURCE_STATE::INDEX_BUFFER);

			//Material index
			gpuMesh->materialIndex = cpuMesh.materialIndex;

			gpuModel->meshes.push_back(std::move(gpuMesh));
		}

		//Load the material data
		for (const CPUMaterial& cpuMaterial : _cpuModel->materials)
		{
			UniquePtr<GPUMaterial> gpuMaterial{ NK_NEW(GPUMaterial) };
			gpuMaterial->lightingModel = cpuMaterial.lightingModel;
			gpuMaterial->textures.resize(std::to_underlying(MODEL_TEXTURE_TYPE::NUM_MODEL_TEXTURE_TYPES));
			gpuMaterial->textureViews.resize(std::to_underlying(MODEL_TEXTURE_TYPE::NUM_MODEL_TEXTURE_TYPES));

			auto createTexture{ [&](const MODEL_TEXTURE_TYPE _textureType)
			{
				const std::size_t index{ std::to_underlying(_textureType) };
				m_imageDataPointers.push(cpuMaterial.allTextures[index]);
				gpuMaterial->textures[index] = m_device.CreateTexture(cpuMaterial.allTextures[index]->desc);
				EnqueueTextureDataUpload(cpuMaterial.allTextures[index]->data, gpuMaterial->textures[index].get(), RESOURCE_STATE::UNDEFINED);
				m_commandBuffer->TransitionBarrier(gpuMaterial->textures[index].get(), RESOURCE_STATE::COPY_DEST, RESOURCE_STATE::SHADER_RESOURCE);

				TextureViewDesc viewDesc{};
				viewDesc.type = TEXTURE_VIEW_TYPE::SHADER_READ_ONLY;
				viewDesc.dimension = TEXTURE_VIEW_DIMENSION::DIM_2;
				viewDesc.format = gpuMaterial->textures[index]->GetFormat();
				gpuMaterial->textureViews[index] = m_device.CreateShaderResourceTextureView(gpuMaterial->textures[index].get(), viewDesc);
			} };

			
			switch (gpuMaterial->lightingModel)
			{
				
			case LIGHTING_MODEL::BLINN_PHONG:
			{
				BlinnPhongMaterial material{ std::get<BlinnPhongMaterial>(cpuMaterial.shaderMaterialData) };
				
				if (material.hasDiffuse)		{ createTexture(static_cast<MODEL_TEXTURE_TYPE>(material.diffuseIdx)); material.diffuseIdx = gpuMaterial->textureViews[material.diffuseIdx]->GetIndex(); }
				if (material.hasSpecular)		{ createTexture(static_cast<MODEL_TEXTURE_TYPE>(material.specularIdx)); material.specularIdx = gpuMaterial->textureViews[material.specularIdx]->GetIndex(); }
				if (material.hasAmbient)		{ createTexture(static_cast<MODEL_TEXTURE_TYPE>(material.ambientIdx)); material.ambientIdx = gpuMaterial->textureViews[material.ambientIdx]->GetIndex(); }
				if (material.hasEmissive)		{ createTexture(static_cast<MODEL_TEXTURE_TYPE>(material.emissiveIdx)); material.emissiveIdx = gpuMaterial->textureViews[material.emissiveIdx]->GetIndex(); }
				if (material.hasNormal)			{ createTexture(static_cast<MODEL_TEXTURE_TYPE>(material.normalIdx)); material.normalIdx = gpuMaterial->textureViews[material.normalIdx]->GetIndex(); }
				if (material.hasShininess)		{ createTexture(static_cast<MODEL_TEXTURE_TYPE>(material.shininessIdx)); material.shininessIdx = gpuMaterial->textureViews[material.shininessIdx]->GetIndex(); }
				if (material.hasOpacity)		{ createTexture(static_cast<MODEL_TEXTURE_TYPE>(material.opacityIdx)); material.opacityIdx = gpuMaterial->textureViews[material.opacityIdx]->GetIndex(); }
				if (material.hasHeight)			{ createTexture(static_cast<MODEL_TEXTURE_TYPE>(material.heightIdx)); material.heightIdx = gpuMaterial->textureViews[material.heightIdx]->GetIndex(); }
				if (material.hasDisplacement)	{ createTexture(static_cast<MODEL_TEXTURE_TYPE>(material.displacementIdx)); material.displacementIdx = gpuMaterial->textureViews[material.displacementIdx]->GetIndex(); }
				if (material.hasLightmap)		{ createTexture(static_cast<MODEL_TEXTURE_TYPE>(material.lightmapIdx)); material.lightmapIdx = gpuMaterial->textureViews[material.lightmapIdx]->GetIndex(); }
				if (material.hasReflection)		{ createTexture(static_cast<MODEL_TEXTURE_TYPE>(material.reflectionIdx)); material.reflectionIdx = gpuMaterial->textureViews[material.reflectionIdx]->GetIndex(); }

				//Material buffer
				BufferDesc materialBufferDesc{};
				materialBufferDesc.size = sizeof(material);
				materialBufferDesc.type = MEMORY_TYPE::DEVICE;
				materialBufferDesc.usage = BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | BUFFER_USAGE_FLAGS::UNIFORM_BUFFER_BIT;
				gpuMaterial->materialBuffer = m_device.CreateBuffer(materialBufferDesc);
				EnqueueBufferDataUpload(&material, gpuMaterial->materialBuffer.get(), RESOURCE_STATE::UNDEFINED);
				m_commandBuffer->TransitionBarrier(gpuMaterial->materialBuffer.get(), RESOURCE_STATE::COPY_DEST, RESOURCE_STATE::CONSTANT_BUFFER);

				//Material buffer view
				BufferViewDesc materialBufferViewDesc{};
				materialBufferViewDesc.size = sizeof(material);
				materialBufferViewDesc.type = BUFFER_VIEW_TYPE::UNIFORM;
				materialBufferViewDesc.offset = 0;
				gpuMaterial->materialBufferView = m_device.CreateBufferView(gpuMaterial->materialBuffer.get(), materialBufferViewDesc);
				gpuMaterial->bufferIndex = gpuMaterial->materialBufferView->GetIndex();
				
				break;
			}
			
			case LIGHTING_MODEL::PHYSICALLY_BASED:
			{
				PBRMaterial material{ std::get<PBRMaterial>(cpuMaterial.shaderMaterialData) };
				
				if (material.hasBaseColour)		{ createTexture(static_cast<MODEL_TEXTURE_TYPE>(material.baseColourIdx)); material.baseColourIdx = gpuMaterial->textureViews[material.baseColourIdx]->GetIndex(); }
				if (material.hasMetalness)		{ createTexture(static_cast<MODEL_TEXTURE_TYPE>(material.metalnessIdx)); material.metalnessIdx = gpuMaterial->textureViews[material.metalnessIdx]->GetIndex(); }
				if (material.hasRoughness)		{ createTexture(static_cast<MODEL_TEXTURE_TYPE>(material.roughnessIdx)); material.roughnessIdx = gpuMaterial->textureViews[material.roughnessIdx]->GetIndex(); }
				if (material.hasSpecular)		{ createTexture(static_cast<MODEL_TEXTURE_TYPE>(material.specularIdx)); material.specularIdx = gpuMaterial->textureViews[material.specularIdx]->GetIndex(); }
				if (material.hasShininess)		{ createTexture(static_cast<MODEL_TEXTURE_TYPE>(material.shininessIdx)); material.shininessIdx = gpuMaterial->textureViews[material.shininessIdx]->GetIndex(); }
				if (material.hasNormal)			{ createTexture(static_cast<MODEL_TEXTURE_TYPE>(material.normalIdx)); material.normalIdx = gpuMaterial->textureViews[material.normalIdx]->GetIndex(); }
				if (material.hasAO)				{ createTexture(static_cast<MODEL_TEXTURE_TYPE>(material.aoIdx)); material.aoIdx = gpuMaterial->textureViews[material.aoIdx]->GetIndex(); }
				if (material.hasEmissive)		{ createTexture(static_cast<MODEL_TEXTURE_TYPE>(material.emissiveIdx)); material.emissiveIdx = gpuMaterial->textureViews[material.emissiveIdx]->GetIndex(); }
				if (material.hasOpacity)		{ createTexture(static_cast<MODEL_TEXTURE_TYPE>(material.opacityIdx)); material.opacityIdx = gpuMaterial->textureViews[material.opacityIdx]->GetIndex(); }
				if (material.hasHeight)			{ createTexture(static_cast<MODEL_TEXTURE_TYPE>(material.heightIdx)); material.heightIdx = gpuMaterial->textureViews[material.heightIdx]->GetIndex(); }
				if (material.hasDisplacement)	{ createTexture(static_cast<MODEL_TEXTURE_TYPE>(material.displacementIdx)); material.displacementIdx = gpuMaterial->textureViews[material.displacementIdx]->GetIndex(); }
				if (material.hasReflection)		{ createTexture(static_cast<MODEL_TEXTURE_TYPE>(material.reflectionIdx)); material.reflectionIdx = gpuMaterial->textureViews[material.reflectionIdx]->GetIndex(); }

				//Material buffer
				BufferDesc materialBufferDesc{};
				materialBufferDesc.size = sizeof(material);
				materialBufferDesc.type = MEMORY_TYPE::DEVICE;
				materialBufferDesc.usage = BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | BUFFER_USAGE_FLAGS::UNIFORM_BUFFER_BIT;
				gpuMaterial->materialBuffer = m_device.CreateBuffer(materialBufferDesc);
				EnqueueBufferDataUpload(&material, gpuMaterial->materialBuffer.get(), RESOURCE_STATE::UNDEFINED);

				//Material buffer view
				BufferViewDesc materialBufferViewDesc{};
				materialBufferViewDesc.size = sizeof(material);
				materialBufferViewDesc.type = BUFFER_VIEW_TYPE::UNIFORM;
				materialBufferViewDesc.offset = 0;
				gpuMaterial->materialBufferView = m_device.CreateBufferView(gpuMaterial->materialBuffer.get(), materialBufferViewDesc);
				gpuMaterial->bufferIndex = gpuMaterial->materialBufferView->GetIndex();
				
				break;
			}
				
			}
			

			gpuModel->materials.push_back(std::move(gpuMaterial));
		}

		return std::move(gpuModel);
	}



	void GPUUploader::Reset()
	{
		if (!m_flushing)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::GPU_UPLOADER, "Attempted to call Reset() while m_flushing was false (did you call Reset() twice?).\n");
			throw std::runtime_error("");
		}
		
		m_stagingBufferSubregions.clear();
		while (!m_imageDataPointers.empty())
		{
			m_imageDataPointers.pop();
		}
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