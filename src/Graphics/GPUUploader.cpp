#include "GPUUploader.h"

#include <cstring>
#include <ranges>
#include <stdexcept>
#include <Core/Utils/ImageLoader.h>
#include <Core/Utils/RHIUtils.h>
#include <RHI/IBuffer.h>
#include <RHI/ICommandBuffer.h>
#include <RHI/IQueue.h>
#include <RHI/ITexture.h>
#include <RHI/ITextureView.h>

#include "RHI/IBufferView.h"


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



	void GPUUploader::EnqueueBufferDataUpload(const void* _data, IBuffer* _dstBuffer, RESOURCE_STATE _dstBufferInitialState)
	{
		if (m_flushing)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::GPU_UPLOADER, "EnqueueBufferDataUploaded() called while m_flushing was true. Did you forget to call Reset()?\n");
			throw std::runtime_error("");
		}
		
		BufferSubregion subregion{};
		subregion.offset = (m_stagingBufferSubregions.empty() ? 0 : m_stagingBufferSubregions.back().offset + m_stagingBufferSubregions.back().size);
		subregion.size = _dstBuffer->GetSize();

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



	void GPUUploader::EnqueueTextureDataUpload(const void* _data, ITexture* _dstTexture, RESOURCE_STATE _dstTextureInitialState)
	{
		if (m_flushing)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::GPU_UPLOADER, "EnqueueTextureDataUploaded() called while m_flushing was true. Did you forget to call Reset()?\n");
			throw std::runtime_error("");
		}
		
		TextureCopyMemoryLayout memLayout{ m_device.GetRequiredMemoryLayoutForTextureCopy(_dstTexture) };

		BufferSubregion subregion{};
		subregion.offset = (m_stagingBufferSubregions.empty() ? 0 : m_stagingBufferSubregions.back().offset + m_stagingBufferSubregions.back().size);
		subregion.size = memLayout.totalBytes;

		if (subregion.offset + subregion.size > m_stagingBufferSize)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::GPU_UPLOADER, "EnqueueTextureDataUpload() exceeded m_stagingBufferSize - flushing staging buffer with _waitIdle = true.\n");
			Flush(true);
			Reset();

			subregion.offset = 0;
		}
		
		//memcpy data one row at a time, advancing ptr by memLayout.rowPitch after each row
		const unsigned char* srcPtr{ static_cast<const unsigned char*>(_data) };
		unsigned char* dstPtr{ m_stagingBufferMap + subregion.offset };
		std::uint32_t bytesPerRow{ RHIUtils::GetFormatBytesPerPixel(_dstTexture->GetFormat()) * _dstTexture->GetSize().x };
		for (std::size_t row{ 0 }; row < _dstTexture->GetSize().y; ++row)
		{
			memcpy(dstPtr, srcPtr, bytesPerRow);
			srcPtr += bytesPerRow;
			dstPtr += memLayout.rowPitch;
		}
		m_stagingBufferSubregions.push_back(subregion);

		if (_dstTextureInitialState != RESOURCE_STATE::COPY_DEST)
		{
			m_commandBuffer->TransitionBarrier(_dstTexture, _dstTextureInitialState, RESOURCE_STATE::COPY_DEST);
		}

		m_commandBuffer->CopyBufferToTexture(m_stagingBuffer.get(), _dstTexture, subregion.offset, { 0, 0, 0 }, _dstTexture->GetSize());
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

			//Index buffer
			BufferDesc indexBufferDesc{};
			indexBufferDesc.size = sizeof(std::uint32_t) * cpuMesh.indices.size();
			indexBufferDesc.type = MEMORY_TYPE::DEVICE;
			indexBufferDesc.usage = BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | BUFFER_USAGE_FLAGS::INDEX_BUFFER_BIT;
			gpuMesh->indexBuffer = m_device.CreateBuffer(indexBufferDesc);
			gpuMesh->indexCount = cpuMesh.indices.size();
			EnqueueBufferDataUpload(cpuMesh.indices.data(), gpuMesh->indexBuffer.get(), RESOURCE_STATE::UNDEFINED);

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
				gpuMaterial->textures[index] = m_device.CreateTexture(cpuMaterial.allTextures[index].desc);
				EnqueueTextureDataUpload(cpuMaterial.allTextures[index].data, gpuMaterial->textures[index].get(), RESOURCE_STATE::UNDEFINED);

				TextureViewDesc viewDesc{};
				viewDesc.type = TEXTURE_VIEW_TYPE::SHADER_READ_ONLY;
				viewDesc.dimension = TEXTURE_DIMENSION::DIM_2;
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
		
		m_flushing = true;
		m_queue->Submit(m_commandBuffer.get(), nullptr, nullptr, fence.get());

		if (_waitIdle)
		{
			m_queue->WaitIdle();
		}
		
		return std::move(fence);
	}

}