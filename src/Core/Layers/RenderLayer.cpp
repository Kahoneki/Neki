#include "RenderLayer.h"

#include <Components/CModelRenderer.h>
#include <Components/CSkybox.h>
#include <Components/CTransform.h>
#include <RHI/IBuffer.h>
#include <RHI/IBufferView.h>
#include <RHI/IQueue.h>
#include <RHI/ISampler.h>
#include <RHI/ISemaphore.h>
#include <RHI/ISurface.h>
#include <RHI/ISwapchain.h>

#ifdef NEKI_VULKAN_SUPPORTED
	#include <RHI-Vulkan/VulkanDevice.h>
#endif
#ifdef NEKI_D3D12_SUPPORTED
	#include <RHI-D3D12/D3D12Device.h>
#endif

#include <cstring>


namespace NK
{

	RenderLayer::RenderLayer(Registry& _reg, const RenderLayerDesc& _desc)
	: ILayer(_reg), m_allocator(*Context::GetAllocator()), m_desc(_desc), m_currentFrame(0), m_supersampleResolution(glm::ivec2(m_desc.ssaaMultiplier) * m_desc.window->GetSize())
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::RENDER_LAYER, "Initialising Render Layer\n");


		InitBaseResources();

		//For resource transition
		m_graphicsCommandBuffers[0]->Begin();

		InitCameraBuffer();
		InitLightCameraBuffer();
		InitSkybox();
		InitShadersAndPipelines();
		InitRenderGraphs();
		InitScreenResources();

		m_graphicsCommandBuffers[0]->End();
		m_graphicsQueue->Submit(m_graphicsCommandBuffers[0].get(), nullptr, nullptr, nullptr);
		m_graphicsQueue->WaitIdle();

		m_gpuUploader->Flush(true, nullptr, nullptr);
		m_gpuUploader->Reset();

		m_modelUnloadQueues.resize(m_desc.framesInFlight);

		
		m_logger.Unindent();
	}



	RenderLayer::~RenderLayer()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::RENDER_LAYER, "Shutting Down Render Layer\n");


		m_graphicsQueue->WaitIdle();


		m_logger.Unindent();
	}



	void RenderLayer::Update()
	{
		//Update skybox
		bool found{ false };
		for (auto&& [skybox] : m_reg.get().View<CSkybox>())
		{
			if (found)
			{
				//Multiple skyboxes, notify the user only the first one is being considered for rendering
				m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::RENDER_LAYER, "Multiple `CSkybox`s found in registry. Note that only one skybox is supported - only the first skybox will be used.\n");
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
//			m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::RENDER_LAYER, "No `CSkybox`s found in registry.\n");
		}


		//Update camera
		found = false;
		for (auto&& [camera] : m_reg.get().View<CCamera>())
		{
			if (found)
			{
				//Multiple cameras, notify the user only the first one is being considered for rendering - todo: change this (probably let the user specify on CModelRenderer which camera they want to use)
				m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::RENDER_LAYER, "Multiple `CCamera`s found in registry. Note that currently, only one camera is supported - only the first camera will be used.\n");
				break;
			}
			found = true;
			UpdateCameraBuffer(camera);
		}
		if (!found)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::RENDER_LAYER, "No `CCamera`s found in registry.\n");
		}


		//Model Loading Phase
		for (auto&& [modelRenderer] : m_reg.get().View<CModelRenderer>())
		{
			if (modelRenderer.visible && !modelRenderer.model)
			{
				//Model is visible but isn't loaded, load it
				const CPUModel* const cpuModel{ ModelLoader::LoadModel(modelRenderer.modelPath, true, true) };
				m_gpuModelCache[modelRenderer.modelPath] = m_gpuUploader->EnqueueModelDataUpload(cpuModel);
				m_newGPUUploaderUpload = true;
				modelRenderer.model = m_gpuModelCache[modelRenderer.modelPath].get();
			}
			else if (!modelRenderer.visible && modelRenderer.model)
			{
				//Model isn't visible but is loaded, add it to the unload queue
				m_modelUnloadQueues[m_currentFrame].push(modelRenderer.modelPath);
				ModelLoader::UnloadModel(modelRenderer.modelPath);
				modelRenderer.model = nullptr;
			}
		}
		//This is the last point in the frame there can be any new gpu uploads, if there are any, start flushing them now and use the fence to wait on before drawing
		if (m_newGPUUploaderUpload)
		{
			m_gpuUploader->Flush(false, m_gpuUploaderFlushFence.get(), nullptr);
		}


		//Begin rendering
		m_inFlightFences[m_currentFrame]->Wait();
		m_inFlightFences[m_currentFrame]->Reset();

		//Fence has been signalled, unload models that were marked for unloading from m_desc.framesInFlight frames ago
		while (!m_modelUnloadQueues[m_currentFrame].empty())
		{
			m_gpuModelCache.erase(m_modelUnloadQueues[m_currentFrame].front());
			m_modelUnloadQueues[m_currentFrame].pop();
		}
		

		m_graphicsCommandBuffers[m_currentFrame]->Begin();
		const std::uint32_t imageIndex{ m_swapchain->AcquireNextImageIndex(m_imageAvailableSemaphores[m_currentFrame].get(), nullptr) };
		
		RenderGraphExecutionDesc execDesc{};
		execDesc.commandBuffers["SHADOW_PASS"] = m_graphicsCommandBuffers[m_currentFrame].get();
		execDesc.commandBuffers["SCENE_PASS"] = m_graphicsCommandBuffers[m_currentFrame].get();
		execDesc.commandBuffers["DOWNSAMPLE_PASS"] = m_graphicsCommandBuffers[m_currentFrame].get();
		execDesc.commandBuffers["PRESENT_TRANSITION_PASS"] = m_graphicsCommandBuffers[m_currentFrame].get();
		execDesc.buffers.Set("LIGHT_CAMERA_BUFFER", m_lightCamDataBuffer.get());
		execDesc.buffers.Set("CAMERA_BUFFER", m_camDataBuffer.get());
		execDesc.textures.Set("SHADOW_MAP", m_shadowMap.get());
		execDesc.textures.Set("SCENE_COLOUR", m_intermediateRenderTarget.get());
		execDesc.textures.Set("SCENE_DEPTH", m_intermediateDepthBuffer.get());
		execDesc.textures.Set("BACKBUFFER", m_swapchain->GetImage(imageIndex));
		execDesc.textures.Set("SKYBOX", m_skyboxTexture.get());
		execDesc.bufferViews.Set("LIGHT_CAMERA_BUFFER_VIEW", m_lightCamDataBufferView.get());
		execDesc.bufferViews.Set("CAMERA_BUFFER_VIEW", m_camDataBufferView.get());
		execDesc.textureViews.Set("SHADOW_MAP_DSV", m_shadowMapDSV.get());
		execDesc.textureViews.Set("SHADOW_MAP_SRV", m_shadowMapSRV.get());
		execDesc.textureViews.Set("SCENE_COLOUR_VIEW", m_intermediateRenderTargetView.get());
		execDesc.textureViews.Set("SCENE_DEPTH_VIEW", m_intermediateDepthBufferView.get());
		execDesc.textureViews.Set("BACKBUFFER_VIEW", m_swapchain->GetImageView(imageIndex));
		execDesc.textureViews.Set("SKYBOX_VIEW", m_skyboxTextureView.get());
		execDesc.samplers.Set("SAMPLER", m_linearSampler.get());
		m_meshRenderGraph->Execute(execDesc);
		
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

		m_currentFrame = (m_currentFrame + 1) % m_desc.framesInFlight;
	}



	void RenderLayer::InitBaseResources()
	{
		//Initialise device
		switch (m_desc.backend)
		{
		case GRAPHICS_BACKEND::VULKAN:
		{
			#ifdef NEKI_VULKAN_SUPPORTED
				m_device = UniquePtr<IDevice>(NK_NEW(VulkanDevice, m_logger, m_allocator));
				#else
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::RENDER_LAYER, "_desc.backend = GRAPHICS_BACKEND::VULKAN but compiler definition NEKI_VULKAN_SUPPORTED is not defined - are you building for the correct cmake preset?\n");
				throw std::invalid_argument("");
			#endif
			break;
		}
		case GRAPHICS_BACKEND::D3D12:
		{
			#ifdef NEKI_D3D12_SUPPORTED
			m_device = UniquePtr<IDevice>(NK_NEW(D3D12Device, m_logger, m_allocator));
			#else
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::RENDER_LAYER, "_desc.backend = GRAPHICS_BACKEND::D3D12 but compiler definition NEKI_D3D12_SUPPORTED is not defined - are you building for the correct cmake preset?\n");
			throw std::invalid_argument("");
			#endif
			break;
		}
		default:
		{
			m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::RENDER_LAYER, "_desc.backend (" + std::to_string(std::to_underlying(m_desc.backend)) + ") not recognised.\n");
			throw std::invalid_argument("");
		}
		}


		//Graphics Command Pool
		CommandPoolDesc graphicsCommandPoolDesc{};
		graphicsCommandPoolDesc.type = COMMAND_TYPE::GRAPHICS;
		m_graphicsCommandPools.resize(m_desc.framesInFlight);
		for (std::size_t i{ 0 }; i < m_desc.framesInFlight; ++i)
		{
			m_graphicsCommandPools[i] = m_device->CreateCommandPool(graphicsCommandPoolDesc);
		}

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
		gpuUploaderDesc.graphicsQueue = m_graphicsQueue.get();
		m_gpuUploader = m_device->CreateGPUUploader(gpuUploaderDesc);

		//GPU Uploader Flush Fence
		FenceDesc gpuUploaderFlushFenceDesc{};
		gpuUploaderFlushFenceDesc.initiallySignaled = false;
		m_gpuUploaderFlushFence = m_device->CreateFence(gpuUploaderFlushFenceDesc);
		m_newGPUUploaderUpload = false;

		//Surface
		m_surface = m_device->CreateSurface(m_desc.window);

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
		m_graphicsCommandBuffers.resize(m_desc.framesInFlight);
		for (std::size_t i{ 0 }; i < m_desc.framesInFlight; ++i)
		{
			m_graphicsCommandBuffers[i] = m_graphicsCommandPools[i]->AllocateCommandBuffer(primaryLevelCommandBufferDesc);
		}

		//Semaphores
		m_imageAvailableSemaphores.resize(m_desc.framesInFlight);
		for (std::size_t i{ 0 }; i < m_desc.framesInFlight; ++i)
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
		m_inFlightFences.resize(m_desc.framesInFlight);
		for (std::size_t i{ 0 }; i < m_desc.framesInFlight; ++i)
		{
			m_inFlightFences[i] = m_device->CreateFence(inFlightFenceDesc);
		}
	}



	void RenderLayer::InitCameraBuffer()
	{
		BufferDesc camDataBufferDesc{};
		camDataBufferDesc.size = sizeof(CameraShaderData);
		camDataBufferDesc.type = MEMORY_TYPE::HOST; //Todo: look into device-local host-accessible memory type?
		camDataBufferDesc.usage = BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | BUFFER_USAGE_FLAGS::UNIFORM_BUFFER_BIT;
		m_camDataBuffer = m_device->CreateBuffer(camDataBufferDesc);
		m_graphicsCommandBuffers[0]->TransitionBarrier(m_camDataBuffer.get(), RESOURCE_STATE::UNDEFINED, RESOURCE_STATE::CONSTANT_BUFFER);

		BufferViewDesc camDataBufferViewDesc{};
		camDataBufferViewDesc.size = sizeof(CameraShaderData);
		camDataBufferViewDesc.offset = 0;
		camDataBufferViewDesc.type = BUFFER_VIEW_TYPE::UNIFORM;
		m_camDataBufferView = m_device->CreateBufferView(m_camDataBuffer.get(), camDataBufferViewDesc);

		m_camDataBufferMap = m_camDataBuffer->GetMap();
	}



	void RenderLayer::InitLightCameraBuffer()
	{
		BufferDesc lightCamDataBufferDesc{};
		lightCamDataBufferDesc.size = sizeof(CameraShaderData);
		lightCamDataBufferDesc.type = MEMORY_TYPE::HOST; //Todo: look into device-local host-accessible memory type?
		lightCamDataBufferDesc.usage = BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | BUFFER_USAGE_FLAGS::UNIFORM_BUFFER_BIT;
		m_lightCamDataBuffer = m_device->CreateBuffer(lightCamDataBufferDesc);
		m_graphicsCommandBuffers[0]->TransitionBarrier(m_lightCamDataBuffer.get(), RESOURCE_STATE::UNDEFINED, RESOURCE_STATE::CONSTANT_BUFFER);

		BufferViewDesc lightCamDataBufferViewDesc{};
		lightCamDataBufferViewDesc.size = sizeof(CameraShaderData);
		lightCamDataBufferViewDesc.offset = 0;
		lightCamDataBufferViewDesc.type = BUFFER_VIEW_TYPE::UNIFORM;
		m_lightCamDataBufferView = m_device->CreateBufferView(m_lightCamDataBuffer.get(), lightCamDataBufferViewDesc);

		m_lightCamDataBufferMap = m_lightCamDataBuffer->GetMap();

		struct LightCameraShaderData
		{
			glm::mat4 viewMat;
			glm::mat4 projMat;
		};

		//Todo: turn this into a loop for all lights and like ykno use a Light struct and whatnot
		LightCameraShaderData lightCamShaderData{};
		lightCamShaderData.viewMat = glm::lookAtLH(glm::vec3(2, -2, 2), glm::vec3(0, 0, 0), glm::vec3(0,1,0));
		lightCamShaderData.projMat = glm::orthoLH(-100.0f, 100.0f, -100.0f, 100.0f, 0.01f, 100.0f);
		memcpy(m_lightCamDataBufferMap, &lightCamShaderData, sizeof(LightCameraShaderData));
	}



	void RenderLayer::InitSkybox()
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

		m_graphicsCommandBuffers[0]->TransitionBarrier(m_skyboxVertBuffer.get(), RESOURCE_STATE::COPY_DEST, RESOURCE_STATE::VERTEX_BUFFER);
		m_graphicsCommandBuffers[0]->TransitionBarrier(m_skyboxIndexBuffer.get(), RESOURCE_STATE::COPY_DEST, RESOURCE_STATE::INDEX_BUFFER);
	}



	void RenderLayer::InitShadersAndPipelines()
	{
		//Vertex Shaders
		ShaderDesc vertShaderDesc{};
		vertShaderDesc.type = SHADER_TYPE::VERTEX;
		vertShaderDesc.filepath = "Shaders/Shadow_vs";
		m_shadowVertShader = m_device->CreateShader(vertShaderDesc);
		vertShaderDesc.filepath = "Shaders/Model_vs";
		m_meshVertShader = m_device->CreateShader(vertShaderDesc);
		vertShaderDesc.filepath = "Shaders/Skybox_vs";
		m_skyboxVertShader = m_device->CreateShader(vertShaderDesc);

		//Fragment Shaders
		ShaderDesc fragShaderDesc{};
		fragShaderDesc.type = SHADER_TYPE::FRAGMENT;
		fragShaderDesc.filepath = "Shaders/Shadow_fs";
		m_shadowFragShader = m_device->CreateShader(fragShaderDesc);
		fragShaderDesc.filepath = "Shaders/ModelBlinnPhong_fs";
		m_blinnPhongFragShader = m_device->CreateShader(fragShaderDesc);
		fragShaderDesc.filepath = "Shaders/ModelPBR_fs";
		m_pbrFragShader = m_device->CreateShader(fragShaderDesc);
		fragShaderDesc.filepath = "Shaders/Skybox_fs";
		m_skyboxFragShader = m_device->CreateShader(fragShaderDesc);


		//Root Signatures
		RootSignatureDesc rootSigDesc{};
		rootSigDesc.num32BitPushConstantValues = sizeof(MeshPassPushConstantData) / 4;
		m_meshPassRootSignature = m_device->CreateRootSignature(rootSigDesc);

		rootSigDesc.num32BitPushConstantValues = sizeof(ShadowPassPushConstantData) / 4;
		m_shadowPassRootSignature = m_device->CreateRootSignature(rootSigDesc);


		InitShadowPipeline();
		InitSkyboxPipeline();
		InitGraphicsPipelines();


	}



	void RenderLayer::InitShadowPipeline()
	{
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
		multisamplingDesc.sampleCount = m_desc.enableMSAA ? m_desc.msaaSampleCount : SAMPLE_COUNT::BIT_1;
		multisamplingDesc.sampleMask = UINT32_MAX;
		multisamplingDesc.alphaToCoverageEnable = false;

		std::vector<ColourBlendAttachmentDesc> colourBlendAttachments{};
		ColourBlendDesc colourBlendDesc{};
		colourBlendDesc.logicOpEnable = false;
		colourBlendDesc.attachments = colourBlendAttachments;

		PipelineDesc pipelineDesc{};
		pipelineDesc.type = PIPELINE_TYPE::GRAPHICS;
		pipelineDesc.vertexShader = m_shadowVertShader.get();
		pipelineDesc.fragmentShader = m_shadowFragShader.get();
		pipelineDesc.rootSignature = m_meshPassRootSignature.get();
		pipelineDesc.vertexInputDesc = vertexInputDesc;
		pipelineDesc.inputAssemblyDesc = inputAssemblyDesc;
		pipelineDesc.rasteriserDesc = rasteriserDesc;
		pipelineDesc.depthStencilDesc = depthStencilDesc;
		pipelineDesc.multisamplingDesc = multisamplingDesc;
		pipelineDesc.colourBlendDesc = colourBlendDesc;
		pipelineDesc.colourAttachmentFormats = {};
		pipelineDesc.depthStencilAttachmentFormat = DATA_FORMAT::D32_SFLOAT;

		m_shadowPipeline = m_device->CreatePipeline(pipelineDesc);
	}



	void RenderLayer::InitSkyboxPipeline()
	{
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
		VertexInputDesc vertexInputDesc{};
		vertexInputDesc.attributeDescriptions = vertexAttributes;
		vertexInputDesc.bufferBindingDescriptions = bufferBindings;

		InputAssemblyDesc inputAssemblyDesc{};
		inputAssemblyDesc.topology = INPUT_TOPOLOGY::TRIANGLE_LIST;

		RasteriserDesc rasteriserDesc{};
		rasteriserDesc.cullMode = CULL_MODE::FRONT;
		rasteriserDesc.frontFace = WINDING_DIRECTION::CLOCKWISE;
		rasteriserDesc.depthBiasEnable = false;

		DepthStencilDesc depthStencilDesc{};
		depthStencilDesc.depthTestEnable = true;
		depthStencilDesc.depthWriteEnable = true;
		depthStencilDesc.depthCompareOp = COMPARE_OP::LESS_OR_EQUAL;
		depthStencilDesc.stencilTestEnable = false;

		MultisamplingDesc multisamplingDesc{};
		multisamplingDesc.sampleCount = m_desc.enableMSAA ? m_desc.msaaSampleCount : SAMPLE_COUNT::BIT_1;
		multisamplingDesc.sampleMask = UINT32_MAX;
		multisamplingDesc.alphaToCoverageEnable = false;

		std::vector<ColourBlendAttachmentDesc> colourBlendAttachments(1);
		colourBlendAttachments[0].colourWriteMask = COLOUR_ASPECT_FLAGS::R_BIT | COLOUR_ASPECT_FLAGS::G_BIT | COLOUR_ASPECT_FLAGS::B_BIT | COLOUR_ASPECT_FLAGS::A_BIT;
		colourBlendAttachments[0].blendEnable = false;
		ColourBlendDesc colourBlendDesc{};
		colourBlendDesc.logicOpEnable = false;
		colourBlendDesc.attachments = colourBlendAttachments;

		PipelineDesc pipelineDesc{};
		pipelineDesc.type = PIPELINE_TYPE::GRAPHICS;
		pipelineDesc.vertexShader = m_skyboxVertShader.get();
		pipelineDesc.fragmentShader = m_skyboxFragShader.get();
		pipelineDesc.rootSignature = m_meshPassRootSignature.get();
		pipelineDesc.vertexInputDesc = vertexInputDesc;
		pipelineDesc.inputAssemblyDesc = inputAssemblyDesc;
		pipelineDesc.rasteriserDesc = rasteriserDesc;
		pipelineDesc.depthStencilDesc = depthStencilDesc;
		pipelineDesc.multisamplingDesc = multisamplingDesc;
		pipelineDesc.colourBlendDesc = colourBlendDesc;
		pipelineDesc.colourAttachmentFormats = { DATA_FORMAT::R8G8B8A8_SRGB };
		pipelineDesc.depthStencilAttachmentFormat = DATA_FORMAT::D32_SFLOAT;

		m_skyboxPipeline = m_device->CreatePipeline(pipelineDesc);
	}



	void RenderLayer::InitGraphicsPipelines()
	{
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
		multisamplingDesc.sampleCount = m_desc.enableMSAA ? m_desc.msaaSampleCount : SAMPLE_COUNT::BIT_1;
		multisamplingDesc.sampleMask = UINT32_MAX;
		multisamplingDesc.alphaToCoverageEnable = false;

		std::vector<ColourBlendAttachmentDesc> colourBlendAttachments(1);
		colourBlendAttachments[0].colourWriteMask = COLOUR_ASPECT_FLAGS::R_BIT | COLOUR_ASPECT_FLAGS::G_BIT | COLOUR_ASPECT_FLAGS::B_BIT | COLOUR_ASPECT_FLAGS::A_BIT;
		colourBlendAttachments[0].blendEnable = false;
		ColourBlendDesc colourBlendDesc{};
		colourBlendDesc.logicOpEnable = false;
		colourBlendDesc.attachments = colourBlendAttachments;

		PipelineDesc pipelineDesc{};
		pipelineDesc.type = PIPELINE_TYPE::GRAPHICS;
		pipelineDesc.vertexShader = m_meshVertShader.get();
		pipelineDesc.fragmentShader = m_blinnPhongFragShader.get();
		pipelineDesc.rootSignature = m_meshPassRootSignature.get();
		pipelineDesc.vertexInputDesc = vertexInputDesc;
		pipelineDesc.inputAssemblyDesc = inputAssemblyDesc;
		pipelineDesc.rasteriserDesc = rasteriserDesc;
		pipelineDesc.depthStencilDesc = depthStencilDesc;
		pipelineDesc.multisamplingDesc = multisamplingDesc;
		pipelineDesc.colourBlendDesc = colourBlendDesc;
		pipelineDesc.colourAttachmentFormats = { DATA_FORMAT::R8G8B8A8_SRGB };
		pipelineDesc.depthStencilAttachmentFormat = DATA_FORMAT::D32_SFLOAT;

		m_blinnPhongPipeline = m_device->CreatePipeline(pipelineDesc);

		pipelineDesc.fragmentShader = m_pbrFragShader.get();
		m_pbrPipeline = m_device->CreatePipeline(pipelineDesc);
	}



	void RenderLayer::InitRenderGraphs()
	{
		RenderGraphDesc meshDesc{};

		meshDesc.AddNode(
			"SHADOW_PASS",
			{{ "LIGHT_CAMERA_BUFFER", RESOURCE_STATE::CONSTANT_BUFFER },
			{ "SHADOW_MAP", RESOURCE_STATE::DEPTH_WRITE }},
			[&](ICommandBuffer* _cmdBuf, const BindingMap<IBuffer>& _bufs, const BindingMap<ITexture>& _texs, const BindingMap<IBufferView>& _bufViews, const BindingMap<ITextureView>& _texViews, const BindingMap<ISampler>& _samplers)
			{
				_cmdBuf->BeginRendering(0, nullptr, nullptr, _texViews.Get("SHADOW_MAP_DSV"), nullptr);
				_cmdBuf->BindRootSignature(m_shadowPassRootSignature.get(), PIPELINE_BIND_POINT::GRAPHICS);

				_cmdBuf->SetViewport({ 0, 0 }, { m_desc.enableSSAA ? m_supersampleResolution : m_desc.window->GetSize() });
				_cmdBuf->SetScissor({ 0, 0 }, { m_desc.enableSSAA ? m_supersampleResolution : m_desc.window->GetSize() });

				ShadowPassPushConstantData pushConstantData{};
				pushConstantData.lightCamDataBufferIndex = _bufViews.Get("LIGHT_CAMERA_BUFFER_VIEW")->GetIndex();
				
				//Models
				std::size_t modelVertexBufferStride{ sizeof(ModelVertex) };
				for (auto&& [modelRenderer, transform] : m_reg.get().View<CModelRenderer, CTransform>())
				{
					if (!modelRenderer.visible) { continue; } //todo: this is obviously wrong
					pushConstantData.modelMat = transform.GetModelMatrix();


					const GPUModel* const model{ modelRenderer.model };
					for (std::size_t i{ 0 }; i < modelRenderer.model->meshes.size(); ++i)
					{
						const GPUMesh* mesh{ model->meshes[i].get() };

						_cmdBuf->PushConstants(m_shadowPassRootSignature.get(), &pushConstantData);
						_cmdBuf->BindPipeline(m_shadowPipeline.get(), PIPELINE_BIND_POINT::GRAPHICS);
						_cmdBuf->BindVertexBuffers(0, 1, model->meshes[i]->vertexBuffer.get(), &modelVertexBufferStride);
						_cmdBuf->BindIndexBuffer(model->meshes[i]->indexBuffer.get(), DATA_FORMAT::R32_UINT);
						_cmdBuf->DrawIndexed(model->meshes[i]->indexCount, 1, 0, 0);
					}
				}

				_cmdBuf->EndRendering(0, nullptr, nullptr);
			}
		);


		meshDesc.AddNode(
		"SCENE_PASS",
		{{ "CAMERA_BUFFER", RESOURCE_STATE::CONSTANT_BUFFER },
		{ "SHADOW_MAP", RESOURCE_STATE::SHADER_RESOURCE },
		{ "SCENE_COLOUR", RESOURCE_STATE::RENDER_TARGET },
		{ "SCENE_DEPTH", RESOURCE_STATE::DEPTH_WRITE },
		{ "BACKBUFFER", RESOURCE_STATE::RENDER_TARGET },
		{ "SKYBOX", RESOURCE_STATE::SHADER_RESOURCE }},
		[&](ICommandBuffer* _cmdBuf, const BindingMap<IBuffer>& _bufs, const BindingMap<ITexture>& _texs, const BindingMap<IBufferView>& _bufViews, const BindingMap<ITextureView>& _texViews, const BindingMap<ISampler>& _samplers)
		{
			_cmdBuf->BeginRendering(1, m_desc.enableMSAA ? _texViews.Get("SCENE_COLOUR_VIEW") : nullptr, m_desc.enableSSAA ? _texViews.Get("SCENE_COLOUR_VIEW") : _texViews.Get("BACKBUFFER_VIEW"), _texViews.Get("SCENE_DEPTH_VIEW"), nullptr);
			_cmdBuf->BindRootSignature(m_meshPassRootSignature.get(), PIPELINE_BIND_POINT::GRAPHICS);

			_cmdBuf->SetViewport({ 0, 0 }, { m_desc.enableSSAA ? m_supersampleResolution : m_desc.window->GetSize() });
			_cmdBuf->SetScissor({ 0, 0 }, { m_desc.enableSSAA ? m_supersampleResolution : m_desc.window->GetSize() });

			MeshPassPushConstantData pushConstantData{};
			pushConstantData.shadowMapIndex = _texViews.Get("SHADOW_MAP_SRV")->GetIndex();
			pushConstantData.camDataBufferIndex = _bufViews.Get("CAMERA_BUFFER_VIEW")->GetIndex();
			pushConstantData.skyboxCubemapIndex = _texViews.Get("SKYBOX_VIEW") ? _texViews.Get("SKYBOX_VIEW")->GetIndex() : 0;
			pushConstantData.samplerIndex = _samplers.Get("SAMPLER")->GetIndex();

			//Skybox
			if (_texs.Get("SKYBOX"))
			{
				std::size_t skyboxVertexBufferStride{ sizeof(glm::vec3) };
				_cmdBuf->PushConstants(m_meshPassRootSignature.get(), &pushConstantData);
				_cmdBuf->BindPipeline(m_skyboxPipeline.get(), PIPELINE_BIND_POINT::GRAPHICS);
				_cmdBuf->BindVertexBuffers(0, 1, m_skyboxVertBuffer.get(), &skyboxVertexBufferStride);
				_cmdBuf->BindIndexBuffer(m_skyboxIndexBuffer.get(), DATA_FORMAT::R32_UINT);
				_cmdBuf->DrawIndexed(36, 1, 0, 0);
			}

			//Models
			std::size_t modelVertexBufferStride{ sizeof(ModelVertex) };
			for (auto&& [modelRenderer, transform] : m_reg.get().View<CModelRenderer, CTransform>())
			{
				if (!modelRenderer.visible) { continue; }
				pushConstantData.modelMat = transform.GetModelMatrix();


				const GPUModel* const model{ modelRenderer.model };
				for (std::size_t i{ 0 }; i < modelRenderer.model->meshes.size(); ++i)
				{
					const GPUMesh* mesh{ model->meshes[i].get() };

					pushConstantData.materialBufferIndex = model->materials[mesh->materialIndex]->bufferIndex;
					_cmdBuf->PushConstants(m_meshPassRootSignature.get(), &pushConstantData);
					IPipeline* pipeline{ model->materials[mesh->materialIndex]->lightingModel == LIGHTING_MODEL::BLINN_PHONG ? m_blinnPhongPipeline.get() : m_pbrPipeline.get() };
					_cmdBuf->BindPipeline(pipeline, PIPELINE_BIND_POINT::GRAPHICS);
					_cmdBuf->BindVertexBuffers(0, 1, model->meshes[i]->vertexBuffer.get(), &modelVertexBufferStride);
					_cmdBuf->BindIndexBuffer(model->meshes[i]->indexBuffer.get(), DATA_FORMAT::R32_UINT);
					_cmdBuf->DrawIndexed(model->meshes[i]->indexCount, 1, 0, 0);
				}
			}

			_cmdBuf->EndRendering(1, m_desc.enableMSAA ? _texs.Get("SCENE_COLOUR") : nullptr, _texs.Get("BACKBUFFER"));
		});

		
		meshDesc.AddNode(
		"DOWNSAMPLE_PASS",
		{{"SCENE_COLOUR", RESOURCE_STATE::COPY_SOURCE},
		{"BACKBUFFER", RESOURCE_STATE::COPY_DEST}},
		[&](ICommandBuffer* _cmdBuf, const BindingMap<IBuffer>& _bufs, const BindingMap<ITexture>& _texs, const BindingMap<IBufferView>& _bufViews, const BindingMap<ITextureView>& _texViews, const BindingMap<ISampler>& _samplers)
		{
			if (m_desc.enableSSAA)
			{
				_cmdBuf->BlitTexture(_texs.Get("SCENE_COLOUR"), TEXTURE_ASPECT::COLOUR, _texs.Get("BACKBUFFER"), TEXTURE_ASPECT::COLOUR);
			}
		});


		meshDesc.AddNode("PRESENT_TRANSITION_PASS", {{"BACKBUFFER", RESOURCE_STATE::PRESENT}});
		
		
		m_meshRenderGraph = UniquePtr<RenderGraph>(NK_NEW(RenderGraph, meshDesc));
	}



	void RenderLayer::InitScreenResources()
	{
		//Shadow Map
		TextureDesc shadowMapDesc{};
		shadowMapDesc.dimension = TEXTURE_DIMENSION::DIM_2;
		shadowMapDesc.format = DATA_FORMAT::D32_SFLOAT;
		shadowMapDesc.size = m_desc.enableSSAA ? glm::ivec3(m_supersampleResolution, 1) : glm::ivec3(m_desc.window->GetSize(), 1);
		shadowMapDesc.usage = TEXTURE_USAGE_FLAGS::DEPTH_STENCIL_ATTACHMENT | TEXTURE_USAGE_FLAGS::READ_ONLY;
		shadowMapDesc.arrayTexture = false;
		shadowMapDesc.sampleCount = m_desc.enableMSAA ? m_desc.msaaSampleCount : SAMPLE_COUNT::BIT_1;
		m_shadowMap = m_device->CreateTexture(shadowMapDesc);
		m_graphicsCommandBuffers[0]->TransitionBarrier(m_shadowMap.get(), RESOURCE_STATE::UNDEFINED, RESOURCE_STATE::DEPTH_WRITE);

		//Shadow Map Views
		TextureViewDesc shadowMapViewDesc{};
		shadowMapViewDesc.dimension = TEXTURE_VIEW_DIMENSION::DIM_2;
		shadowMapViewDesc.format = DATA_FORMAT::D32_SFLOAT;
		shadowMapViewDesc.type = TEXTURE_VIEW_TYPE::DEPTH;
		m_shadowMapDSV = m_device->CreateDepthStencilTextureView(m_shadowMap.get(), shadowMapViewDesc);
		shadowMapViewDesc.type = TEXTURE_VIEW_TYPE::SHADER_READ_ONLY;
		m_shadowMapSRV = m_device->CreateShaderResourceTextureView(m_shadowMap.get(), shadowMapViewDesc);

		//Render Target
		TextureDesc renderTargetDesc{};
		renderTargetDesc.dimension = TEXTURE_DIMENSION::DIM_2;
		renderTargetDesc.format = DATA_FORMAT::R8G8B8A8_SRGB;
		renderTargetDesc.size = m_desc.enableSSAA ? glm::ivec3(m_supersampleResolution, 1) : glm::ivec3(m_desc.window->GetSize(), 1);
		renderTargetDesc.usage = TEXTURE_USAGE_FLAGS::COLOUR_ATTACHMENT | TEXTURE_USAGE_FLAGS::TRANSFER_SRC_BIT; //Needs TRANSFER_SRC_BIT for supersampling algorithm
		renderTargetDesc.arrayTexture = false;
		renderTargetDesc.sampleCount = m_desc.enableMSAA ? m_desc.msaaSampleCount : SAMPLE_COUNT::BIT_1;
		m_intermediateRenderTarget = m_device->CreateTexture(renderTargetDesc);
		m_graphicsCommandBuffers[0]->TransitionBarrier(m_intermediateRenderTarget.get(), RESOURCE_STATE::UNDEFINED, RESOURCE_STATE::RENDER_TARGET);

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
		depthBufferDesc.size = m_desc.enableSSAA ? glm::ivec3(m_supersampleResolution, 1) : glm::ivec3(m_desc.window->GetSize(), 1);
		depthBufferDesc.usage = TEXTURE_USAGE_FLAGS::DEPTH_STENCIL_ATTACHMENT;
		depthBufferDesc.arrayTexture = false;
		depthBufferDesc.sampleCount = m_desc.enableMSAA ? m_desc.msaaSampleCount : SAMPLE_COUNT::BIT_1;
		m_intermediateDepthBuffer = m_device->CreateTexture(depthBufferDesc);
		m_graphicsCommandBuffers[0]->TransitionBarrier(m_intermediateDepthBuffer.get(), RESOURCE_STATE::UNDEFINED, RESOURCE_STATE::DEPTH_WRITE);

		//Supersample Depth Buffer View
		TextureViewDesc depthBufferViewDesc{};
		depthBufferViewDesc.dimension = TEXTURE_VIEW_DIMENSION::DIM_2;
		depthBufferViewDesc.format = DATA_FORMAT::D32_SFLOAT;
		depthBufferViewDesc.type = TEXTURE_VIEW_TYPE::DEPTH;
		m_intermediateDepthBufferView = m_device->CreateDepthStencilTextureView(m_intermediateDepthBuffer.get(), depthBufferViewDesc);
	}



	void RenderLayer::UpdateSkybox(CSkybox& _skybox)
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



	void RenderLayer::UpdateCameraBuffer(const CCamera& _camera) const
	{
		//Check if the user has requested that the camera's aspect ratio be the window's aspect ratio
		constexpr float epsilon{ 0.1f }; //Buffer for floating-point-comparison-imprecision mitigation
		if (std::abs(_camera.camera->GetAspectRatio() - WIN_ASPECT_RATIO) < epsilon)
		{
			_camera.camera->SetAspectRatio(static_cast<float>(m_desc.window->GetSize().x) / m_desc.window->GetSize().y);
		}
		
		const CameraShaderData camShaderData{ _camera.camera->GetCameraShaderData(PROJECTION_METHOD::PERSPECTIVE) };
		memcpy(m_camDataBufferMap, &camShaderData, sizeof(CameraShaderData));

		//Set aspect ratio back to WIN_ASPECT_RATIO for future lookups (can't keep it as the window's current aspect ratio in case it changes)
		_camera.camera->SetAspectRatio(WIN_ASPECT_RATIO);
	}

}