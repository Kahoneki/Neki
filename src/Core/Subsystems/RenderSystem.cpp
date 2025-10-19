#include "RenderSystem.h"

#include <Components/CModelRenderer.h>
#include <Components/CSkybox.h>
#include <Components/CTransform.h>
#include <RHI/IBuffer.h>
#include <RHI/ISampler.h>
#include <RHI/ISemaphore.h>
#include <RHI/ISurface.h>
#include <RHI/ISwapchain.h>
#include <RHI/IQueue.h>

#ifdef NEKI_VULKAN_SUPPORTED
	#include <RHI-Vulkan/VulkanDevice.h>
#endif
#ifdef NEKI_D3D12_SUPPORTED
	#include <RHI-D3D12/D3D12Device.h>
#endif

#include <cstring>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>


namespace NK
{

	RenderSystem::RenderSystem(ILogger& _logger, IAllocator& _allocator, const RenderSystemDesc& _desc)
	: m_logger(_logger), m_allocator(_allocator), m_backend(_desc.backend), m_msaaEnabled(_desc.enableMSAA), m_msaaSampleCount(_desc.msaaSampleCount), m_ssaaEnabled(_desc.enableSSAA), m_ssaaMultiplier(_desc.ssaaMultiplier), m_windowDesc(_desc.windowDesc), m_framesInFlight(_desc.framesInFlight), m_currentFrame(0), m_supersampleResolution(glm::ivec2(m_ssaaMultiplier) * m_windowDesc.size)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::RENDER_SYSTEM, "Initialising Render System\n");


		InitBaseResources();
		InitCameraBuffer();
		InitSkybox();
		InitShadersAndPipelines();
		InitAntiAliasingResources();

		m_gpuUploader->Flush(true, nullptr, nullptr);
		m_gpuUploader->Reset();

		
		m_logger.Unindent();
	}



	RenderSystem::~RenderSystem()
	{
		m_graphicsQueue->WaitIdle();
	}



	void RenderSystem::Update(Registry& _reg)
	{
		m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::RENDER_SYSTEM, "Draw\n");

		//Update skybox
		bool found{ false };
		for (auto&& [skybox] : _reg.View<CSkybox>())
		{
			if (found)
			{
				//Multiple skyboxes, notify the user only the first one is being considered for rendering
				m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::RENDER_SYSTEM, "Multiple `CSkybox`s found in registry. Note that only one skybox is supported - only the first skybox will be used.\n");
				break;
			}
			found = true;
			if (skybox.dirty)
			{
				UpdateSkybox(skybox);
			}
		}
		if (!found)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::RENDER_SYSTEM, "No `CCamera`s found in registry.\n");
		}


		//Update camera
		found = false;
		for (auto&& [camera] : _reg.View<CCamera>())
		{
			if (found)
			{
				//Multiple cameras, notify the user only the first one is being considered for rendering - todo: change this (probably let the user specify on CModelRenderer which camera they want to use)
				m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::RENDER_SYSTEM, "Multiple `CCamera`s found in registry. Note that currently, only one camera is supported - only the first camera will be used.\n");
				break;
			}
			found = true;
			UpdateCameraBuffer(camera);
		}
		if (!found)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::RENDER_SYSTEM, "No `CCamera`s found in registry.\n");
		}


		//Begin rendering
		m_inFlightFences[m_currentFrame]->Wait();
		m_inFlightFences[m_currentFrame]->Reset();

		m_graphicsCommandBuffers[m_currentFrame]->Begin();
		const std::uint32_t imageIndex{ m_swapchain->AcquireNextImageIndex(m_imageAvailableSemaphores[m_currentFrame].get(), nullptr) };

		if (m_ssaaEnabled || m_msaaEnabled)
		{
			m_graphicsCommandBuffers[m_currentFrame]->TransitionBarrier(m_intermediateRenderTarget.get(), RESOURCE_STATE::UNDEFINED, RESOURCE_STATE::RENDER_TARGET);
		}
		m_graphicsCommandBuffers[m_currentFrame]->TransitionBarrier(m_swapchain->GetImage(imageIndex), RESOURCE_STATE::UNDEFINED, RESOURCE_STATE::RENDER_TARGET);
		//m_graphicsCommandBuffers[m_currentFrame]->TransitionBarrier(m_intermediateDepthBuffer.get(), RESOURCE_STATE::UNDEFINED, RESOURCE_STATE::DEPTH_WRITE);

		m_graphicsCommandBuffers[m_currentFrame]->BeginRendering(1, m_msaaEnabled ? m_intermediateRenderTargetView.get() : nullptr, m_ssaaEnabled ? m_intermediateRenderTargetView.get() : m_swapchain->GetImageView(imageIndex), m_intermediateDepthBufferView.get(), nullptr);
		m_graphicsCommandBuffers[m_currentFrame]->BindRootSignature(m_meshPiplineRootSignature.get(), PIPELINE_BIND_POINT::GRAPHICS);

		m_graphicsCommandBuffers[m_currentFrame]->SetViewport({ 0, 0 }, { m_ssaaEnabled ? m_supersampleResolution : m_windowDesc.size });
		m_graphicsCommandBuffers[m_currentFrame]->SetScissor({ 0, 0 }, { m_ssaaEnabled ? m_supersampleResolution : m_windowDesc.size });


		//Draw
		struct PushConstantData
		{
			glm::mat4 modelMat;
			ResourceIndex camDataBufferIndex;
			ResourceIndex skyboxCubemapIndex;
			ResourceIndex materialBufferIndex;
			SamplerIndex samplerIndex;
		};

		PushConstantData pushConstantData{};
		pushConstantData.camDataBufferIndex = m_camDataBufferView->GetIndex();
		pushConstantData.skyboxCubemapIndex = m_skyboxTextureView ? m_skyboxTextureView->GetIndex() : 0;
		pushConstantData.samplerIndex = m_linearSampler->GetIndex();

		//Skybox
		if (m_skyboxTexture)
		{
			std::size_t skyboxVertexBufferStride{ sizeof(glm::vec3) };
			m_graphicsCommandBuffers[m_currentFrame]->PushConstants(m_meshPiplineRootSignature.get(), &pushConstantData);
			m_graphicsCommandBuffers[m_currentFrame]->BindPipeline(m_skyboxPipeline.get(), PIPELINE_BIND_POINT::GRAPHICS);
			m_graphicsCommandBuffers[m_currentFrame]->BindVertexBuffers(0, 1, m_skyboxVertBuffer.get(), &skyboxVertexBufferStride);
			m_graphicsCommandBuffers[m_currentFrame]->BindIndexBuffer(m_skyboxIndexBuffer.get(), DATA_FORMAT::R32_UINT);
			m_graphicsCommandBuffers[m_currentFrame]->DrawIndexed(36, 1, 0, 0);
		}

		//Models (Loading Phase)
		for (auto&& [modelRenderer] : _reg.View<CModelRenderer>())
		{
			if (modelRenderer.model == nullptr && !modelRenderer.modelPath.empty())
			{
				if (!m_gpuModelCache.contains(modelRenderer.modelPath))
				{
					const CPUModel* const cpuModel{ ModelLoader::LoadModel(modelRenderer.modelPath, true, true) };
					m_gpuModelCache[modelRenderer.modelPath] = m_gpuUploader->EnqueueModelDataUpload(cpuModel);
					m_newGPUUploaderUpload = true;
				}
				modelRenderer.model = m_gpuModelCache[modelRenderer.modelPath].get();
			}
		}
		//This is the last point in the frame there can be any new gpu uploads, if there are any, start flushing them now and use the semaphore to wait on before drawing
		if (m_newGPUUploaderUpload)
		{
			m_gpuUploader->Flush(false, m_gpuUploaderFlushFence.get(), nullptr);
		}

		//Models (Rendering Phase)
		std::size_t modelVertexBufferStride{ sizeof(ModelVertex) };
		for (auto&& [modelRenderer, transform] : _reg.View<CModelRenderer, CTransform>())
		{
			if (transform.dirty)
			{
				UpdateModelMatrix(transform);
			}
			pushConstantData.modelMat = transform.modelMat;


			const GPUModel* const model{ modelRenderer.model };
			for (std::size_t i{ 0 }; i < modelRenderer.model->meshes.size(); ++i)
			{
				const GPUMesh* mesh{ model->meshes[i].get() };

				pushConstantData.materialBufferIndex = model->materials[mesh->materialIndex]->bufferIndex;
				m_graphicsCommandBuffers[m_currentFrame]->PushConstants(m_meshPiplineRootSignature.get(), &pushConstantData);
				IPipeline* pipeline{ model->materials[mesh->materialIndex]->lightingModel == LIGHTING_MODEL::BLINN_PHONG ? m_blinnPhongPipeline.get() : m_pbrPipeline.get() };
				m_graphicsCommandBuffers[m_currentFrame]->BindPipeline(pipeline, PIPELINE_BIND_POINT::GRAPHICS);
				m_graphicsCommandBuffers[m_currentFrame]->BindVertexBuffers(0, 1, model->meshes[i]->vertexBuffer.get(), &modelVertexBufferStride);
				m_graphicsCommandBuffers[m_currentFrame]->BindIndexBuffer(model->meshes[i]->indexBuffer.get(), DATA_FORMAT::R32_UINT);
				m_graphicsCommandBuffers[m_currentFrame]->DrawIndexed(model->meshes[i]->indexCount, 1, 0, 0);
			}
		}


		m_graphicsCommandBuffers[m_currentFrame]->EndRendering();

		if (m_ssaaEnabled)
		{
			//Downscaling pass
			m_graphicsCommandBuffers[m_currentFrame]->TransitionBarrier(m_intermediateRenderTarget.get(), RESOURCE_STATE::RENDER_TARGET, RESOURCE_STATE::COPY_SOURCE);
			m_graphicsCommandBuffers[m_currentFrame]->TransitionBarrier(m_swapchain->GetImage(imageIndex), RESOURCE_STATE::UNDEFINED, RESOURCE_STATE::COPY_DEST);
			m_graphicsCommandBuffers[m_currentFrame]->BlitTexture(m_intermediateRenderTarget.get(), TEXTURE_ASPECT::COLOUR, m_swapchain->GetImage(imageIndex), TEXTURE_ASPECT::COLOUR);
			m_graphicsCommandBuffers[m_currentFrame]->TransitionBarrier(m_swapchain->GetImage(imageIndex), RESOURCE_STATE::COPY_DEST, RESOURCE_STATE::PRESENT);
		}
		else
		{
			m_graphicsCommandBuffers[m_currentFrame]->TransitionBarrier(m_swapchain->GetImage(imageIndex), RESOURCE_STATE::RENDER_TARGET, RESOURCE_STATE::PRESENT);
		}

		//Present
		m_graphicsCommandBuffers[m_currentFrame]->End();

		if (m_newGPUUploaderUpload)
		{
			m_gpuUploaderFlushFence->Wait();
			m_gpuUploaderFlushFence->Reset();
			m_gpuUploader->Reset();
		}

		//Submit
		m_graphicsQueue->Submit(m_graphicsCommandBuffers[m_currentFrame].get(), m_imageAvailableSemaphores[m_currentFrame].get(), m_renderFinishedSemaphores[imageIndex].get(), m_inFlightFences[m_currentFrame].get());
		m_swapchain->Present(m_renderFinishedSemaphores[imageIndex].get(), imageIndex);

		m_currentFrame = (m_currentFrame + 1) % m_framesInFlight;
	}



	void RenderSystem::InitBaseResources()
	{
		//Initialise device
		switch (m_backend)
		{
		case GRAPHICS_BACKEND::NONE:
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::RENDER_SYSTEM, "_desc.backend = GRAPHICS_BACKEND::NONE, RenderSystem should not have been initialised by Engine - this error message indicates an internal error with the engine, please make a GitHub issue on the topic.\n");
			throw std::invalid_argument("");
		}
		case GRAPHICS_BACKEND::VULKAN:
		{
			#ifdef NEKI_VULKAN_SUPPORTED
			m_device = UniquePtr<IDevice>(NK_NEW(VulkanDevice, m_logger, m_allocator));
			#else
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::RENDER_SYSTEM, "_desc.backend = GRAPHICS_BACKEND::VULKAN but compiler definition NEKI_VULKAN_SUPPORTED is not defined - are you building for the correct cmake preset?\n");
			throw std::invalid_argument("");
			#endif
			break;
		}
		case GRAPHICS_BACKEND::D3D12:
		{
			#ifdef NEKI_D3D12_SUPPORTED
			m_device = UniquePtr<IDevice>(NK_NEW(D3D12Device, m_logger, m_allocator));
			#else
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::RENDER_SYSTEM, "_desc.backend = GRAPHICS_BACKEND::D3D12 but compiler definition NEKI_D3D12_SUPPORTED is not defined - are you building for the correct cmake preset?\n");
			throw std::invalid_argument("");
			#endif
			break;
		}
		default:
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::RENDER_SYSTEM, "_desc.backend (" + std::to_string(std::to_underlying(m_backend)) + ") not recognised.\n");
			throw std::invalid_argument("");
		}
		}


		//Graphics Command Pool
		CommandPoolDesc graphicsCommandPoolDesc{};
		graphicsCommandPoolDesc.type = COMMAND_TYPE::GRAPHICS;
		m_graphicsCommandPool = m_device->CreateCommandPool(graphicsCommandPoolDesc);

		//Primary Level Command Buffer Description
		CommandBufferDesc primaryLevelCommandBufferDesc{};
		primaryLevelCommandBufferDesc.level = COMMAND_BUFFER_LEVEL::PRIMARY;

		//Graphics Queue
		QueueDesc graphicsQueueDesc{};
		graphicsQueueDesc.type = COMMAND_TYPE::GRAPHICS;
		m_graphicsQueue = m_device->CreateQueue(graphicsQueueDesc);

		//Transfer Queue
		QueueDesc transferQueueDesc{};
		transferQueueDesc.type = COMMAND_TYPE::TRANSFER;
		m_transferQueue = m_device->CreateQueue(transferQueueDesc);

		//GPU Uploader
		GPUUploaderDesc gpuUploaderDesc{};
		gpuUploaderDesc.stagingBufferSize = 1024 * 512 * 512; //512MiB
		gpuUploaderDesc.transferQueue = m_transferQueue.get();
		m_gpuUploader = m_device->CreateGPUUploader(gpuUploaderDesc);

		//GPU Uploader Flush Fence
		FenceDesc gpuUploaderFlushFenceDesc{};
		gpuUploaderFlushFenceDesc.initiallySignaled = false;
		m_gpuUploaderFlushFence = m_device->CreateFence(gpuUploaderFlushFenceDesc);
		m_newGPUUploaderUpload = false;

		//Window and Surface
		m_window = m_device->CreateWindow(m_windowDesc);
		m_surface = m_device->CreateSurface(m_window.get());

		//Swapchain
		SwapchainDesc swapchainDesc{};
		swapchainDesc.surface = m_surface.get();
		swapchainDesc.numBuffers = 3;
		swapchainDesc.presentQueue = m_graphicsQueue.get();
		m_swapchain = m_device->CreateSwapchain(swapchainDesc);

		//Sampler
		SamplerDesc samplerDesc{};
		samplerDesc.minFilter = FILTER_MODE::LINEAR;
		samplerDesc.magFilter = FILTER_MODE::LINEAR;
		m_linearSampler = m_device->CreateSampler(samplerDesc);

		//Graphics Command Buffers
		m_graphicsCommandBuffers.resize(m_framesInFlight);
		for (std::size_t i{ 0 }; i < m_framesInFlight; ++i)
		{
			m_graphicsCommandBuffers[i] = m_graphicsCommandPool->AllocateCommandBuffer(primaryLevelCommandBufferDesc);
		}

		//Semaphores
		m_imageAvailableSemaphores.resize(m_framesInFlight);
		for (std::size_t i{ 0 }; i < m_framesInFlight; ++i)
		{
			m_imageAvailableSemaphores[i] = m_device->CreateSemaphore();
		}
		m_renderFinishedSemaphores.resize(m_swapchain->GetNumImages());
		for (std::size_t i{ 0 }; i < m_swapchain->GetNumImages(); ++i)
		{
			m_renderFinishedSemaphores[i] = m_device->CreateSemaphore();
		}

		//Fences
		FenceDesc inFlightFenceDesc{};
		inFlightFenceDesc.initiallySignaled = true;
		m_inFlightFences.resize(m_framesInFlight);
		for (std::size_t i{ 0 }; i < m_framesInFlight; ++i)
		{
			m_inFlightFences[i] = m_device->CreateFence(inFlightFenceDesc);
		}
	}



	void RenderSystem::InitCameraBuffer()
	{
		BufferDesc camDataBufferDesc{};
		camDataBufferDesc.size = sizeof(CameraShaderData);
		camDataBufferDesc.type = MEMORY_TYPE::HOST; //Todo: look into device-local host-accessible memory type?
		camDataBufferDesc.usage = BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | BUFFER_USAGE_FLAGS::UNIFORM_BUFFER_BIT;
		m_camDataBuffer = m_device->CreateBuffer(camDataBufferDesc);

		BufferViewDesc camDataBufferViewDesc{};
		camDataBufferViewDesc.size = sizeof(CameraShaderData);
		camDataBufferViewDesc.offset = 0;
		camDataBufferViewDesc.type = BUFFER_VIEW_TYPE::UNIFORM;
		m_camDataBufferView = m_device->CreateBufferView(m_camDataBuffer.get(), camDataBufferViewDesc);

		m_camDataBufferMap = m_camDataBuffer->Map();
	}



	void RenderSystem::InitSkybox()
	{
		//Creating m_skyboxTexture and m_skyboxTextureView requires knowing the size of the skybox which cannot be known at startup, skip its creation

		//Vertex Buffer
		constexpr glm::vec3 vertices[8] =
		{
			glm::vec3(-1.0f, -1.0f, 1.0f), //0: Front-Left-Bottom
			glm::vec3(1.0f, -1.0f, 1.0f), //1: Front-Right-Bottom
			glm::vec3(1.0f, 1.0f, 1.0f), //2: Front-Right-Top
			glm::vec3(-1.0f, 1.0f, 1.0f), //3: Front-Left-Top
			glm::vec3(-1.0f, -1.0f, -1.0f), //4: Back-Left-Bottom
			glm::vec3(1.0f, -1.0f, -1.0f), //5: Back-Right-Bottom
			glm::vec3(1.0f, 1.0f, -1.0f), //6: Back-Right-Top
			glm::vec3(-1.0f, 1.0f, -1.0f)  //7: Back-Left-Top
		};
		BufferDesc skyboxVertBufferDesc{};
		skyboxVertBufferDesc.size = sizeof(glm::vec3) * 8;
		skyboxVertBufferDesc.type = MEMORY_TYPE::DEVICE;
		skyboxVertBufferDesc.usage = BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | BUFFER_USAGE_FLAGS::VERTEX_BUFFER_BIT;
		m_skyboxVertBuffer = m_device->CreateBuffer(skyboxVertBufferDesc);

		//Index Buffer
		const std::uint32_t indices[36] =
		{
			//Front face
			0, 1, 2,
			2, 3, 0,

			//Right face
			1, 5, 6,
			6, 2, 1,

			//Back face
			7, 6, 5,
			5, 4, 7,

			//Left face
			4, 0, 3,
			3, 7, 4,

			//Top face
			3, 2, 6,
			6, 7, 3,

			//Bottom face
			4, 5, 1,
			1, 0, 4
		};
		BufferDesc indexBufferDesc{};
		indexBufferDesc.size = sizeof(std::uint32_t) * 36;
		indexBufferDesc.type = MEMORY_TYPE::DEVICE;
		indexBufferDesc.usage = BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | BUFFER_USAGE_FLAGS::INDEX_BUFFER_BIT;
		m_skyboxIndexBuffer = m_device->CreateBuffer(indexBufferDesc);

		//Upload vertex and index buffers
		m_gpuUploader->EnqueueBufferDataUpload(vertices, m_skyboxVertBuffer.get(), RESOURCE_STATE::UNDEFINED);
		m_gpuUploader->EnqueueBufferDataUpload(indices, m_skyboxIndexBuffer.get(), RESOURCE_STATE::UNDEFINED);
	}



	void RenderSystem::InitShadersAndPipelines()
	{
		//Vertex Shaders
		ShaderDesc vertShaderDesc{};
		vertShaderDesc.type = SHADER_TYPE::VERTEX;
		vertShaderDesc.filepath = "Shaders/Model_vs";
		m_meshVertShader = m_device->CreateShader(vertShaderDesc);
		vertShaderDesc.filepath = "Shaders/Skybox_vs";
		m_skyboxVertShader = m_device->CreateShader(vertShaderDesc);

		//Fragment Shaders
		ShaderDesc fragShaderDesc{};
		fragShaderDesc.type = SHADER_TYPE::FRAGMENT;
		fragShaderDesc.filepath = "Shaders/ModelBlinnPhong_fs";
		m_blinnPhongFragShader = m_device->CreateShader(fragShaderDesc);
		fragShaderDesc.filepath = "Shaders/ModelPBR_fs";
		m_pbrFragShader = m_device->CreateShader(fragShaderDesc);
		fragShaderDesc.filepath = "Shaders/Skybox_fs";
		m_skyboxFragShader = m_device->CreateShader(fragShaderDesc);

		//Root Signature
		RootSignatureDesc rootSigDesc{};
		rootSigDesc.num32BitPushConstantValues = 16 + 1 + 1 + 1 + 1 + 1; //model matrix + cam data buffer index + skybox cubemap index + material buffer index + sampler index
		m_meshPiplineRootSignature = m_device->CreateRootSignature(rootSigDesc);


		//Graphics Pipeline
		VertexInputDesc vertexInputDesc{ ModelLoader::GetModelVertexInputDescription() };

		InputAssemblyDesc inputAssemblyDesc{};
		inputAssemblyDesc.topology = INPUT_TOPOLOGY::TRIANGLE_LIST;

		RasteriserDesc rasteriserDesc{};
		rasteriserDesc.cullMode = CULL_MODE::BACK;
		rasteriserDesc.frontFace = WINDING_DIRECTION::CLOCKWISE;
		rasteriserDesc.depthBiasEnable = false;

		DepthStencilDesc depthStencilDesc{};
		depthStencilDesc.depthTestEnable = true;
		depthStencilDesc.depthWriteEnable = true;
		depthStencilDesc.depthCompareOp = COMPARE_OP::LESS;
		depthStencilDesc.stencilTestEnable = false;

		MultisamplingDesc multisamplingDesc{};
		multisamplingDesc.sampleCount = m_msaaEnabled ? m_msaaSampleCount : SAMPLE_COUNT::BIT_1;
		multisamplingDesc.sampleMask = UINT32_MAX;
		multisamplingDesc.alphaToCoverageEnable = false;

		std::vector<ColourBlendAttachmentDesc> colourBlendAttachments(1);
		colourBlendAttachments[0].colourWriteMask = COLOUR_ASPECT_FLAGS::R_BIT | COLOUR_ASPECT_FLAGS::G_BIT | COLOUR_ASPECT_FLAGS::B_BIT | COLOUR_ASPECT_FLAGS::A_BIT;
		colourBlendAttachments[0].blendEnable = false;
		ColourBlendDesc colourBlendDesc{};
		colourBlendDesc.logicOpEnable = false;
		colourBlendDesc.attachments = colourBlendAttachments;

		PipelineDesc graphicsPipelineDesc{};
		graphicsPipelineDesc.type = PIPELINE_TYPE::GRAPHICS;
		graphicsPipelineDesc.vertexShader = m_meshVertShader.get();
		graphicsPipelineDesc.fragmentShader = m_blinnPhongFragShader.get();
		graphicsPipelineDesc.rootSignature = m_meshPiplineRootSignature.get();
		graphicsPipelineDesc.vertexInputDesc = vertexInputDesc;
		graphicsPipelineDesc.inputAssemblyDesc = inputAssemblyDesc;
		graphicsPipelineDesc.rasteriserDesc = rasteriserDesc;
		graphicsPipelineDesc.depthStencilDesc = depthStencilDesc;
		graphicsPipelineDesc.multisamplingDesc = multisamplingDesc;
		graphicsPipelineDesc.colourBlendDesc = colourBlendDesc;
		graphicsPipelineDesc.colourAttachmentFormats = { DATA_FORMAT::R8G8B8A8_SRGB };
		graphicsPipelineDesc.depthStencilAttachmentFormat = DATA_FORMAT::D32_SFLOAT;

		m_blinnPhongPipeline = m_device->CreatePipeline(graphicsPipelineDesc);

		graphicsPipelineDesc.fragmentShader = m_pbrFragShader.get();
		m_pbrPipeline = m_device->CreatePipeline(graphicsPipelineDesc);


		//Skybox pipeline
		std::vector<VertexAttributeDesc> vertexAttributes;
		VertexAttributeDesc posAttribute{};
		posAttribute.attribute = SHADER_ATTRIBUTE::POSITION;
		posAttribute.binding = 0;
		posAttribute.format = DATA_FORMAT::R32G32B32_SFLOAT;
		posAttribute.offset = 0;
		vertexAttributes.push_back(posAttribute);
		std::vector<VertexBufferBindingDesc> bufferBindings;
		VertexBufferBindingDesc bufferBinding{};
		bufferBinding.binding = 0;
		bufferBinding.inputRate = VERTEX_INPUT_RATE::VERTEX;
		bufferBinding.stride = sizeof(glm::vec3);
		bufferBindings.push_back(bufferBinding);
		VertexInputDesc skyboxVertexInputDesc{};
		skyboxVertexInputDesc.attributeDescriptions = vertexAttributes;
		skyboxVertexInputDesc.bufferBindingDescriptions = bufferBindings;
		graphicsPipelineDesc.vertexInputDesc = skyboxVertexInputDesc;
		graphicsPipelineDesc.rasteriserDesc.cullMode = CULL_MODE::FRONT;
		graphicsPipelineDesc.depthStencilDesc.depthCompareOp = COMPARE_OP::LESS_OR_EQUAL;
		graphicsPipelineDesc.vertexShader = m_skyboxVertShader.get();
		graphicsPipelineDesc.fragmentShader = m_skyboxFragShader.get();
		m_skyboxPipeline = m_device->CreatePipeline(graphicsPipelineDesc);
	}



	void RenderSystem::InitAntiAliasingResources()
	{
		//Render Target
		TextureDesc renderTargetDesc{};
		renderTargetDesc.dimension = TEXTURE_DIMENSION::DIM_2;
		renderTargetDesc.format = DATA_FORMAT::R8G8B8A8_SRGB;
		renderTargetDesc.size = m_ssaaEnabled ? glm::ivec3(m_supersampleResolution, 1) : glm::ivec3(m_windowDesc.size, 1);
		renderTargetDesc.usage = TEXTURE_USAGE_FLAGS::COLOUR_ATTACHMENT | TEXTURE_USAGE_FLAGS::TRANSFER_SRC_BIT; //Needs TRANSFER_SRC_BIT for supersampling algorithm
		renderTargetDesc.arrayTexture = false;
		renderTargetDesc.sampleCount = m_msaaEnabled ? m_msaaSampleCount : SAMPLE_COUNT::BIT_1;
		m_intermediateRenderTarget = m_device->CreateTexture(renderTargetDesc);

		//Render Target View
		TextureViewDesc renderTargetViewDesc{};
		renderTargetViewDesc.dimension = TEXTURE_VIEW_DIMENSION::DIM_2;
		renderTargetViewDesc.format = DATA_FORMAT::R8G8B8A8_SRGB;
		renderTargetViewDesc.type = TEXTURE_VIEW_TYPE::RENDER_TARGET;
		m_intermediateRenderTargetView = m_device->CreateRenderTargetTextureView(m_intermediateRenderTarget.get(), renderTargetViewDesc);

		//Depth Buffer
		TextureDesc depthBufferDesc{};
		depthBufferDesc.dimension = TEXTURE_DIMENSION::DIM_2;
		depthBufferDesc.format = DATA_FORMAT::D32_SFLOAT;
		depthBufferDesc.size = m_ssaaEnabled ? glm::ivec3(m_supersampleResolution, 1) : glm::ivec3(m_windowDesc.size, 1);
		depthBufferDesc.usage = TEXTURE_USAGE_FLAGS::DEPTH_STENCIL_ATTACHMENT;
		depthBufferDesc.arrayTexture = false;
		depthBufferDesc.sampleCount = m_msaaEnabled ? m_msaaSampleCount : SAMPLE_COUNT::BIT_1;
		m_intermediateDepthBuffer = m_device->CreateTexture(depthBufferDesc);

		//Supersample Depth Buffer View
		TextureViewDesc depthBufferViewDesc{};
		depthBufferViewDesc.dimension = TEXTURE_VIEW_DIMENSION::DIM_2;
		depthBufferViewDesc.format = DATA_FORMAT::D32_SFLOAT;
		depthBufferViewDesc.type = TEXTURE_VIEW_TYPE::DEPTH;
		m_intermediateDepthBufferView = m_device->CreateDepthStencilTextureView(m_intermediateDepthBuffer.get(), depthBufferViewDesc);
	}



	void RenderSystem::UpdateSkybox(CSkybox& _skybox)
	{
		std::array<std::string, 6> textureNames{ "right", "left", "top", "bottom", "front", "back" };
		void* skyboxImageData[6];
		std::string filepath{};
		for (std::size_t i{ 0 }; i < 6; ++i)
		{
			filepath = _skybox.GetTextureDirectory() + "/" + textureNames[i] + "." + _skybox.GetFileExtension();
			skyboxImageData[i] = ImageLoader::LoadImage(filepath, false, true)->data;
		}
		
		TextureDesc skyboxTextureDesc{ ImageLoader::LoadImage(filepath, false, true)->desc }; //cached, very inexpensive load
		skyboxTextureDesc.usage |= TEXTURE_USAGE_FLAGS::READ_ONLY;
		skyboxTextureDesc.arrayTexture = true;
		skyboxTextureDesc.size.z = 6;
		skyboxTextureDesc.cubemap = true;
		m_skyboxTexture = m_device->CreateTexture(skyboxTextureDesc);
		m_gpuUploader->EnqueueArrayTextureDataUpload(skyboxImageData, m_skyboxTexture.get(), RESOURCE_STATE::UNDEFINED);
		m_newGPUUploaderUpload = true;

		TextureViewDesc skyboxTextureViewDesc{};
		skyboxTextureViewDesc.dimension = TEXTURE_VIEW_DIMENSION::DIM_CUBE;
		skyboxTextureViewDesc.format = DATA_FORMAT::R8G8B8A8_SRGB;
		skyboxTextureViewDesc.type = TEXTURE_VIEW_TYPE::SHADER_READ_ONLY;
		skyboxTextureViewDesc.baseArrayLayer = 0;
		skyboxTextureViewDesc.arrayLayerCount = 6;
		m_skyboxTextureView = m_device->CreateShaderResourceTextureView(m_skyboxTexture.get(), skyboxTextureViewDesc);
		
		_skybox.dirty = false;
	}



	void RenderSystem::UpdateCameraBuffer(const CCamera& _camera) const
	{
		//Check if the user has requested that the camera's aspect ratio be the window's aspect ratio
		constexpr float epsilon{ 0.1f }; //Buffer for floating-point-comparison-imprecision mitigation
		if (std::abs(_camera.camera->GetAspectRatio() - WIN_ASPECT_RATIO) < epsilon)
		{
			_camera.camera->SetAspectRatio(static_cast<float>(m_windowDesc.size.x) / m_windowDesc.size.y);
		}
		
		const CameraShaderData camShaderData{ _camera.camera->GetCameraShaderData(PROJECTION_METHOD::PERSPECTIVE) };
		memcpy(m_camDataBufferMap, &camShaderData, sizeof(CameraShaderData));

		//Set aspect ratio back to WIN_ASPECT_RATIO for future lookups (can't keep it as the window's current aspect ratio in case it changes)
		_camera.camera->SetAspectRatio(WIN_ASPECT_RATIO);
	}



	void RenderSystem::UpdateModelMatrix(CTransform& _transform)
	{
		//Scale -> Rotation -> Translation
		//Because matrix multiplication order is reversed, do trans * rot * scale

		const glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), _transform.scale);
		const glm::mat4 rotMat = glm::mat4_cast(glm::quat(_transform.rot));
		const glm::mat4 transMat = glm::translate(glm::mat4(1.0f), _transform.pos);

		_transform.modelMat = transMat * rotMat * scaleMat;

		_transform.dirty = false;
	}

}