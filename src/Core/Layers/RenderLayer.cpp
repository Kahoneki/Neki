#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "RenderLayer.h"

#include <Components/CLight.h>
#include <Components/CModelRenderer.h>
#include <Components/CSkybox.h>
#include <Components/CTransform.h>
#include <Core/Utils/TextureCompressor.h>
#include <Graphics/Lights/DirectionalLight.h>
#include <Graphics/Lights/PointLight.h>
#include <Graphics/Lights/SpotLight.h>
#include <Managers/TimeManager.h>
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
	: ILayer(_reg), m_allocator(*Context::GetAllocator()), m_desc(_desc), m_currentFrame(0), m_supersampleResolution(glm::ivec2(m_desc.ssaaMultiplier) * m_desc.window->GetSize()), m_firstFrame(true)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::RENDER_LAYER, "Initialising Render Layer\n");


		InitBaseResources();

		//For resource transition
		m_graphicsCommandBuffers[0]->Begin();

		InitCameraBuffers();
		InitLightDataBuffer();
		InitModelMatricesBuffer();
		InitModelVisibilityBuffers();
		InitCube();
		InitScreenQuad();
		InitShadersAndPipelines();
		InitRenderGraphs();
		InitScreenResources();
		InitBRDFLut();

		m_graphicsCommandBuffers[0]->End();
		m_graphicsQueue->Submit(m_graphicsCommandBuffers[0].get(), nullptr, nullptr, nullptr);
		m_graphicsQueue->WaitIdle();

		m_gpuUploader->Flush(true, nullptr, nullptr);
		m_gpuUploader->Reset();

		m_modelUnloadQueues.resize(m_desc.framesInFlight);
		m_modelMatricesEntitiesLookups.resize(m_desc.framesInFlight);

		m_pointLightProjMatrix = glm::perspectiveLH(glm::radians(90.0f), 1.0f, 0.01f, 1000.0f);

		
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
		
		//Begin rendering
		m_inFlightFences[m_currentFrame]->Wait();
		m_inFlightFences[m_currentFrame]->Reset();
		m_graphicsCommandBuffers[m_currentFrame]->Begin();
		
		
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
			UpdateSkybox(skybox);
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


		UpdateLightDataBuffer();


		//Fence has been signalled, unload models that were marked for unloading from m_desc.framesInFlight frames ago
		while (!m_modelUnloadQueues[m_currentFrame].empty())
		{
			ModelLoader::UnloadModel(m_modelUnloadQueues[m_currentFrame].back());
			m_gpuModelCache.erase(m_modelUnloadQueues[m_currentFrame].back());
			m_modelUnloadQueues[m_currentFrame].pop_back();
		}

		std::uint32_t* visibilityMap{ static_cast<std::uint32_t*>(m_modelVisibilityBufferMaps[(m_currentFrame + m_desc.framesInFlight - 1) % m_desc.framesInFlight]) };
		
		//Model Loading Phase
		for (std::size_t i{ 0 }; i < m_modelMatricesEntitiesLookups[m_currentFrame].size(); ++i)
		{
			CModelRenderer& modelRenderer{ m_reg.get().GetComponent<CModelRenderer>(m_modelMatricesEntitiesLookups[m_currentFrame][i]) };
//			modelRenderer.visible = visibilityMap[i] == 1;
			modelRenderer.visible = true;

			if (modelRenderer.visible && !modelRenderer.model)
			{
				//If model is only queued for unload (and so hasn't yet been unloaded), we can just remove it from the unload queue
				bool modelInUnloadQueue{ false };
				for (std::vector<std::string>& q : m_modelUnloadQueues)
				{
					std::vector<std::string>::iterator it{ std::ranges::find(q, modelRenderer.modelPath) };
					if (it != q.end())
					{
						modelRenderer.model = m_gpuModelCache[modelRenderer.modelPath].get();
						modelInUnloadQueue = true;
						q.erase(it);
					}
				}
				if (modelInUnloadQueue)
				{
					continue;
				}
				
				//Model is visible but isn't loaded, load it
				const CPUModel* const cpuModel{ ModelLoader::LoadModel(modelRenderer.modelPath, true, true) };
				m_gpuModelCache[modelRenderer.modelPath] = m_gpuUploader->EnqueueModelDataUpload(cpuModel);
				m_newGPUUploaderUpload = true;
				modelRenderer.model = m_gpuModelCache[modelRenderer.modelPath].get();
			}
			
			else if (!modelRenderer.visible && modelRenderer.model)
			{
				//Model isn't visible but is loaded, add it to the unload queue
				m_modelUnloadQueues[(m_currentFrame + m_desc.framesInFlight - 1) % m_desc.framesInFlight].push_back(modelRenderer.modelPath);
				modelRenderer.model = nullptr;
			}
		}
		//This is the last point in the frame there can be any new gpu uploads, if there are any, start flushing them now and use the fence to wait on before drawing
		if (m_newGPUUploaderUpload)
		{
			m_gpuUploader->Flush(false, m_gpuUploaderFlushFence.get(), nullptr);
		}

		//Clear the visibility buffer
		std::memset(visibilityMap, 0, m_desc.maxModels * sizeof(std::uint32_t));

		UpdateModelMatricesBuffer();
		

		const std::uint32_t imageIndex{ m_swapchain->AcquireNextImageIndex(m_imageAvailableSemaphores[m_currentFrame].get(), nullptr) };

		
		RenderGraphExecutionDesc execDesc{};
		execDesc.commandBuffers["MODEL_VISIBILITY_PASS"] = m_graphicsCommandBuffers[m_currentFrame].get();
		execDesc.commandBuffers["CAMERA_BUFFER_PREVIOUS_FRAME_COPY_PASS"] = m_graphicsCommandBuffers[m_currentFrame].get();
		execDesc.commandBuffers["SHADOW_PASS"] = m_graphicsCommandBuffers[m_currentFrame].get();
		execDesc.commandBuffers["DEPTH_BARRIER"] = m_graphicsCommandBuffers[m_currentFrame].get();
		execDesc.commandBuffers["SCENE_PASS"] = m_graphicsCommandBuffers[m_currentFrame].get();
		if (m_desc.enableMSAA) { execDesc.commandBuffers["MSAA_RESOLVE_PASS"] = m_graphicsCommandBuffers[m_currentFrame].get(); }
		if (m_desc.enableSSAA) { execDesc.commandBuffers["SSAA_DOWNSAMPLE_PASS"] = m_graphicsCommandBuffers[m_currentFrame].get(); }
		execDesc.commandBuffers["POSTPROCESS_PASS"] = m_graphicsCommandBuffers[m_currentFrame].get();
		execDesc.commandBuffers["PRESENT_TRANSITION_PASS"] = m_graphicsCommandBuffers[m_currentFrame].get();

		execDesc.buffers.Set("CAMERA_BUFFER", m_camDataBuffers[m_currentFrame].get());
		execDesc.buffers.Set("CAMERA_BUFFER_PREVIOUS_FRAME", m_camDataBuffersPreviousFrame[m_currentFrame].get());
		execDesc.buffers.Set("LIGHT_DATA_BUFFER", m_lightDataBuffer.get());
		execDesc.buffers.Set("MODEL_MATRICES_BUFFER", m_modelMatricesBuffers[m_currentFrame].get());
		execDesc.buffers.Set("MODEL_VISIBILITY_BUFFER", m_modelVisibilityBuffers[m_currentFrame].get());

		execDesc.textures.Set("SCENE_COLOUR", m_sceneColour.get());
		execDesc.textures.Set("SCENE_COLOUR_MSAA", m_sceneColourMSAA.get());
		execDesc.textures.Set("SCENE_COLOUR_SSAA", m_sceneColourSSAA.get());

		execDesc.textures.Set("SCENE_DEPTH", m_sceneDepth.get());
		execDesc.textures.Set("SCENE_DEPTH_MSAA", m_sceneDepthMSAA.get());
		execDesc.textures.Set("SCENE_DEPTH_SSAA", m_sceneDepthSSAA.get());

		execDesc.textures.Set("BACKBUFFER", m_swapchain->GetImage(imageIndex));
		execDesc.textures.Set("SKYBOX", m_skyboxTexture.get());
		execDesc.textures.Set("IRRADIANCE_MAP", m_irradianceMap.get());
		execDesc.textures.Set("PREFILTER_MAP", m_prefilterMap.get());
		execDesc.textures.Set("BRDF_LUT", m_brdfLUT.get());

		execDesc.bufferViews.Set("LIGHT_DATA_BUFFER_VIEW", m_lightDataBufferView.get());
		execDesc.bufferViews.Set("CAMERA_BUFFER_VIEW", m_camDataBufferViews[m_currentFrame].get());
		execDesc.bufferViews.Set("CAMERA_BUFFER_PREVIOUS_FRAME_VIEW", m_camDataBufferPreviousFrameViews[m_currentFrame].get());
		execDesc.bufferViews.Set("MODEL_MATRICES_BUFFER_VIEW", m_modelMatricesBufferViews[m_currentFrame].get());
		execDesc.bufferViews.Set("MODEL_VISIBILITY_BUFFER_VIEW", m_modelVisibilityBufferViews[m_currentFrame].get());

		execDesc.textureViews.Set("SCENE_COLOUR_RTV", m_sceneColourRTV.get());
		execDesc.textureViews.Set("SCENE_COLOUR_SRV", m_sceneColourSRV.get());
		execDesc.textureViews.Set("SCENE_COLOUR_MSAA_RTV", m_sceneColourMSAARTV.get());
		execDesc.textureViews.Set("SCENE_COLOUR_MSAA_SRV", m_sceneColourMSAASRV.get());
		execDesc.textureViews.Set("SCENE_COLOUR_SSAA_RTV", m_sceneColourSSAARTV.get());
		execDesc.textureViews.Set("SCENE_COLOUR_SSAA_SRV", m_sceneColourSSAASRV.get());

		execDesc.textureViews.Set("SCENE_DEPTH_DSV", m_sceneDepthDSV.get());
		execDesc.textureViews.Set("SCENE_DEPTH_SRV", m_sceneDepthSRV.get());
		execDesc.textureViews.Set("SCENE_DEPTH_MSAA_DSV", m_sceneDepthMSAADSV.get());
		execDesc.textureViews.Set("SCENE_DEPTH_MSAA_SRV", m_sceneDepthMSAASRV.get());
		execDesc.textureViews.Set("SCENE_DEPTH_SSAA_DSV", m_sceneDepthSSAADSV.get());
		execDesc.textureViews.Set("SCENE_DEPTH_SSAA_SRV", m_sceneDepthSSAASRV.get());

		execDesc.textureViews.Set("BACKBUFFER_RTV", m_swapchain->GetImageView(imageIndex));
		execDesc.textureViews.Set("SKYBOX_VIEW", m_skyboxTextureView.get());
		execDesc.textureViews.Set("IRRADIANCE_MAP_VIEW", m_skyboxTextureView.get());
		execDesc.textureViews.Set("PREFILTER_MAP_VIEW", m_skyboxTextureView.get());
		execDesc.textureViews.Set("BRDF_LUT_VIEW", m_skyboxTextureView.get());

		execDesc.samplers.Set("SAMPLER", m_linearSampler.get());
		execDesc.samplers.Set("BRDF_LUT_SAMPLER", m_linearSampler.get());
		
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

		m_firstFrame = false;

		m_graphicsQueue->WaitIdle();
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
		gpuUploaderDesc.stagingBufferSize = 1024 * 1024 * 512; //512MiB
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

		
		//Samplers
		SamplerDesc samplerDesc{};
		samplerDesc.minFilter = FILTER_MODE::LINEAR;
		samplerDesc.magFilter = FILTER_MODE::LINEAR;
		samplerDesc.mipmapFilter = FILTER_MODE::LINEAR;
		samplerDesc.maxAnisotropy = 16.0f;
		m_linearSampler = m_device->CreateSampler(samplerDesc);

		samplerDesc.maxAnisotropy = 1.0f;
		samplerDesc.addressModeU = ADDRESS_MODE::CLAMP_TO_EDGE;
		samplerDesc.addressModeV = ADDRESS_MODE::CLAMP_TO_EDGE;
		m_brdfLUTSampler = m_device->CreateSampler(samplerDesc);
		
		
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



	void RenderLayer::InitCameraBuffers()
	{
		m_camDataBuffers.resize(m_desc.framesInFlight);
		m_camDataBufferViews.resize(m_desc.framesInFlight);
		m_camDataBufferMaps.resize(m_desc.framesInFlight);
		m_camDataBuffersPreviousFrame.resize(m_desc.framesInFlight);
		m_camDataBufferPreviousFrameViews.resize(m_desc.framesInFlight);
		
		BufferDesc camDataBufferDesc{};
		camDataBufferDesc.size = sizeof(CameraShaderData);
		camDataBufferDesc.type = MEMORY_TYPE::HOST; //Todo: look into device-local host-accessible memory type?
		camDataBufferDesc.usage = BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | BUFFER_USAGE_FLAGS::TRANSFER_SRC_BIT | BUFFER_USAGE_FLAGS::UNIFORM_BUFFER_BIT;
		for (std::size_t i{ 0 }; i < m_desc.framesInFlight; ++i)
		{
			m_camDataBuffers[i] = m_device->CreateBuffer(camDataBufferDesc);
			m_camDataBuffersPreviousFrame[i] = m_device->CreateBuffer(camDataBufferDesc);
			m_graphicsCommandBuffers[0]->TransitionBarrier(m_camDataBuffers[i].get(), RESOURCE_STATE::UNDEFINED, RESOURCE_STATE::CONSTANT_BUFFER);
			m_graphicsCommandBuffers[0]->TransitionBarrier(m_camDataBuffersPreviousFrame[i].get(), RESOURCE_STATE::UNDEFINED, RESOURCE_STATE::CONSTANT_BUFFER);
		}
		
		BufferViewDesc camDataBufferViewDesc{};
		camDataBufferViewDesc.size = sizeof(CameraShaderData);
		camDataBufferViewDesc.offset = 0;
		camDataBufferViewDesc.type = BUFFER_VIEW_TYPE::UNIFORM;
		for (std::size_t i{ 0 }; i < m_desc.framesInFlight; ++i)
		{
			m_camDataBufferViews[i] = m_device->CreateBufferView(m_camDataBuffers[i].get(), camDataBufferViewDesc);
			m_camDataBufferPreviousFrameViews[i] = m_device->CreateBufferView(m_camDataBuffersPreviousFrame[i].get(), camDataBufferViewDesc);

			m_camDataBufferMaps[i] = m_camDataBuffers[i]->GetMap();
		}
	}



	void RenderLayer::InitLightDataBuffer()
	{
		BufferDesc lightDataBufferDesc{};
		lightDataBufferDesc.size = sizeof(LightShaderData) * m_desc.maxLights;
		lightDataBufferDesc.type = MEMORY_TYPE::HOST; //Todo: look into device-local host-accessible memory type?
		lightDataBufferDesc.usage = BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | BUFFER_USAGE_FLAGS::STORAGE_BUFFER_BIT;
		m_lightDataBuffer = m_device->CreateBuffer(lightDataBufferDesc);
		m_graphicsCommandBuffers[0]->TransitionBarrier(m_lightDataBuffer.get(), RESOURCE_STATE::UNDEFINED, RESOURCE_STATE::SHADER_RESOURCE);

		BufferViewDesc lightDataBufferViewDesc{};
		lightDataBufferViewDesc.size = sizeof(LightShaderData);
		lightDataBufferViewDesc.offset = 0;
		lightDataBufferViewDesc.type = BUFFER_VIEW_TYPE::STORAGE_READ_ONLY;
		m_lightDataBufferView = m_device->CreateBufferView(m_lightDataBuffer.get(), lightDataBufferViewDesc);

		m_lightDataBufferMap = m_lightDataBuffer->GetMap();
	}



	void RenderLayer::InitModelMatricesBuffer()
	{
		m_modelMatricesBuffers.resize(m_desc.framesInFlight);
		m_modelMatricesBufferViews.resize(m_desc.framesInFlight);
		m_modelMatricesBufferMaps.resize(m_desc.framesInFlight);

		for (std::size_t i = 0; i < m_desc.framesInFlight; ++i)
		{
			BufferDesc modelMatricesBufferDesc{};
			modelMatricesBufferDesc.size = m_desc.maxModels * sizeof(glm::mat4);
			modelMatricesBufferDesc.type = MEMORY_TYPE::HOST; //Todo: look into device-local host-accessible memory type?
			modelMatricesBufferDesc.usage = BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | BUFFER_USAGE_FLAGS::STORAGE_BUFFER_BIT;
			m_modelMatricesBuffers[i] = m_device->CreateBuffer(modelMatricesBufferDesc);
			m_graphicsCommandBuffers[0]->TransitionBarrier(m_modelMatricesBuffers[i].get(), RESOURCE_STATE::UNDEFINED, RESOURCE_STATE::SHADER_RESOURCE);

			BufferViewDesc modelMatricesBufferViewDesc{};
			modelMatricesBufferViewDesc.size = m_desc.maxModels * sizeof(glm::mat4);
			modelMatricesBufferViewDesc.offset = 0;
			modelMatricesBufferViewDesc.type = BUFFER_VIEW_TYPE::STORAGE_READ_ONLY;
			m_modelMatricesBufferViews[i] = m_device->CreateBufferView(m_modelMatricesBuffers[i].get(), modelMatricesBufferViewDesc);

			m_modelMatricesBufferMaps[i] = m_modelMatricesBuffers[i]->GetMap();
		}
	}



	void RenderLayer::InitModelVisibilityBuffers()
	{
		m_modelVisibilityBuffers.resize(m_desc.framesInFlight);
		m_modelVisibilityBufferViews.resize(m_desc.framesInFlight);
		m_modelVisibilityBufferMaps.resize(m_desc.framesInFlight);

		for (std::uint32_t i = 0; i < m_desc.framesInFlight; ++i)
		{
			BufferDesc modelVisibilityBufferDesc{};
			modelVisibilityBufferDesc.size = m_desc.maxModels * sizeof(std::uint32_t);
			modelVisibilityBufferDesc.type = MEMORY_TYPE::HOST; //Todo: look into device-local host-accessible memory type?
			modelVisibilityBufferDesc.usage = BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | BUFFER_USAGE_FLAGS::STORAGE_BUFFER_BIT;
			m_modelVisibilityBuffers[i] = m_device->CreateBuffer(modelVisibilityBufferDesc);
			m_graphicsCommandBuffers[0]->TransitionBarrier(m_modelVisibilityBuffers[i].get(), RESOURCE_STATE::UNDEFINED, RESOURCE_STATE::UNORDERED_ACCESS);

			BufferViewDesc modelVisibilityBufferViewDesc{};
			modelVisibilityBufferViewDesc.size = m_desc.maxModels * sizeof(std::uint32_t);
			modelVisibilityBufferViewDesc.offset = 0;
			modelVisibilityBufferViewDesc.type = BUFFER_VIEW_TYPE::STORAGE_READ_WRITE;
			m_modelVisibilityBufferViews[i] = m_device->CreateBufferView(m_modelVisibilityBuffers[i].get(), modelVisibilityBufferViewDesc);

			m_modelVisibilityBufferMaps[i] = m_modelVisibilityBuffers[i]->GetMap();
			std::memset(m_modelVisibilityBufferMaps[i], 1, m_desc.maxModels * sizeof(std::uint32_t));
		}
	}



	void RenderLayer::InitCube()
	{
		//Vertex Buffer
		constexpr glm::vec3 vertices[8] =
		{
			glm::vec3(-0.5f, -0.5f, 0.5f), //0: Front-Left-Bottom
			glm::vec3(0.5f, -0.5f, 0.5f), //1: Front-Right-Bottom
			glm::vec3(0.5f, 0.5f, 0.5f), //2: Front-Right-Top
			glm::vec3(-0.5f, 0.5f, 0.5f), //3: Front-Left-Top
			glm::vec3(-0.5f, -0.5f, -0.5f), //4: Back-Left-Bottom
			glm::vec3(0.5f, -0.5f, -0.5f), //5: Back-Right-Bottom
			glm::vec3(0.5f, 0.5f, -0.5f), //6: Back-Right-Top
			glm::vec3(-0.5f, 0.5f, -0.5f)  //7: Back-Left-Top
		};
		BufferDesc cubeVertBufferDesc{};
		cubeVertBufferDesc.size = sizeof(glm::vec3) * 8;
		cubeVertBufferDesc.type = MEMORY_TYPE::DEVICE;
		cubeVertBufferDesc.usage = BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | BUFFER_USAGE_FLAGS::VERTEX_BUFFER_BIT;
		m_cubeVertBuffer = m_device->CreateBuffer(cubeVertBufferDesc);

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
		m_cubeIndexBuffer = m_device->CreateBuffer(indexBufferDesc);

		//Upload vertex and index buffers
		m_gpuUploader->EnqueueBufferDataUpload(vertices, m_cubeVertBuffer.get(), RESOURCE_STATE::UNDEFINED);
		m_gpuUploader->EnqueueBufferDataUpload(indices, m_cubeIndexBuffer.get(), RESOURCE_STATE::UNDEFINED);

		m_graphicsCommandBuffers[0]->TransitionBarrier(m_cubeVertBuffer.get(), RESOURCE_STATE::COPY_DEST, RESOURCE_STATE::VERTEX_BUFFER);
		m_graphicsCommandBuffers[0]->TransitionBarrier(m_cubeIndexBuffer.get(), RESOURCE_STATE::COPY_DEST, RESOURCE_STATE::INDEX_BUFFER);
	}



	void RenderLayer::InitScreenQuad()
	{
		//Vertex Buffer
		constexpr ScreenQuadVertex  vertices[4]
		{
			{ glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec2(0.0f, 1.0f) },	//Bottom-left
			{ glm::vec3(-1.0f,  1.0f, 0.0f), glm::vec2(0.0f, 0.0f) },	//Top-left
			{ glm::vec3( 1.0f, -1.0f, 0.0f), glm::vec2(1.0f, 1.0f) },	//Bottom-right
			{ glm::vec3( 1.0f,  1.0f, 0.0f), glm::vec2(1.0f, 0.0f) }	//Top-right
		};
		BufferDesc vertBufferDesc{};
		vertBufferDesc.size = sizeof(ScreenQuadVertex) * 4;
		vertBufferDesc.type = MEMORY_TYPE::DEVICE;
		vertBufferDesc.usage = BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | BUFFER_USAGE_FLAGS::VERTEX_BUFFER_BIT;
		m_screenQuadVertBuffer = m_device->CreateBuffer(vertBufferDesc);

		//Index Buffer
		const std::uint32_t indices[6] =
		{
			0, 1, 2,
			2, 1, 3
		};
		BufferDesc indexBufferDesc{};
		indexBufferDesc.size = sizeof(std::uint32_t) * 6;
		indexBufferDesc.type = MEMORY_TYPE::DEVICE;
		indexBufferDesc.usage = BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | BUFFER_USAGE_FLAGS::INDEX_BUFFER_BIT;
		m_screenQuadIndexBuffer = m_device->CreateBuffer(indexBufferDesc);

		//Upload vertex and index buffers
		m_gpuUploader->EnqueueBufferDataUpload(vertices, m_screenQuadVertBuffer.get(), RESOURCE_STATE::UNDEFINED);
		m_gpuUploader->EnqueueBufferDataUpload(indices, m_screenQuadIndexBuffer.get(), RESOURCE_STATE::UNDEFINED);

		m_graphicsCommandBuffers[0]->TransitionBarrier(m_screenQuadVertBuffer.get(), RESOURCE_STATE::COPY_DEST, RESOURCE_STATE::VERTEX_BUFFER);
		m_graphicsCommandBuffers[0]->TransitionBarrier(m_screenQuadIndexBuffer.get(), RESOURCE_STATE::COPY_DEST, RESOURCE_STATE::INDEX_BUFFER);
	}



	void RenderLayer::InitShadersAndPipelines()
	{
		//Vertex Shaders
		ShaderDesc vertShaderDesc{};
		vertShaderDesc.type = SHADER_TYPE::VERTEX;
		vertShaderDesc.filepath = "Shaders/ModelVisibility_vs";
		m_modelVisibilityVertShader = m_device->CreateShader(vertShaderDesc);
		vertShaderDesc.filepath = "Shaders/Shadow_vs";
		m_shadowVertShader = m_device->CreateShader(vertShaderDesc);
		vertShaderDesc.filepath = "Shaders/Model_vs";
		m_meshVertShader = m_device->CreateShader(vertShaderDesc);
		vertShaderDesc.filepath = "Shaders/Skybox_vs";
		m_skyboxVertShader = m_device->CreateShader(vertShaderDesc);
		vertShaderDesc.filepath = "Shaders/ScreenQuad_vs";
		m_screenQuadVertShader = m_device->CreateShader(vertShaderDesc);

		//Fragment Shaders
		ShaderDesc fragShaderDesc{};
		fragShaderDesc.type = SHADER_TYPE::FRAGMENT;
		fragShaderDesc.filepath = "Shaders/ModelVisibility_fs";
		m_modelVisibilityFragShader = m_device->CreateShader(fragShaderDesc);
		fragShaderDesc.filepath = "Shaders/Shadow_fs";
		m_shadowFragShader = m_device->CreateShader(fragShaderDesc);
		fragShaderDesc.filepath = "Shaders/ModelBlinnPhong_fs";
		m_blinnPhongFragShader = m_device->CreateShader(fragShaderDesc);
		fragShaderDesc.filepath = "Shaders/ModelPBR_fs";
		m_pbrFragShader = m_device->CreateShader(fragShaderDesc);
		fragShaderDesc.filepath = "Shaders/Skybox_fs";
		m_skyboxFragShader = m_device->CreateShader(fragShaderDesc);
		fragShaderDesc.filepath = "Shaders/Postprocess_fs";
		m_postprocessFragShader = m_device->CreateShader(fragShaderDesc);
		

		//Root Signatures
		RootSignatureDesc rootSigDesc{};
		rootSigDesc.num32BitPushConstantValues = sizeof(ModelVisibilityPassPushConstantData) / 4;
		m_modelVisibilityPassRootSignature = m_device->CreateRootSignature(rootSigDesc);
		rootSigDesc.num32BitPushConstantValues = sizeof(ShadowPassPushConstantData) / 4;
		m_shadowPassRootSignature = m_device->CreateRootSignature(rootSigDesc);
		rootSigDesc.num32BitPushConstantValues = sizeof(MeshPassPushConstantData) / 4;
		m_meshPassRootSignature = m_device->CreateRootSignature(rootSigDesc);
		rootSigDesc.num32BitPushConstantValues = sizeof(PostprocessPassPushConstantData) / 4;
		m_postprocessPassRootSignature = m_device->CreateRootSignature(rootSigDesc);
		

		InitModelVisibilityPipeline();
		InitShadowPipeline();
		InitSkyboxPipeline();
		InitGraphicsPipelines();
		InitPostprocessPipeline();
	}



	void RenderLayer::InitModelVisibilityPipeline()
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
		rasteriserDesc.cullMode = CULL_MODE::NONE;
		rasteriserDesc.frontFace = WINDING_DIRECTION::CLOCKWISE;
		rasteriserDesc.depthBiasEnable = false;

		DepthStencilDesc depthStencilDesc{};
		depthStencilDesc.depthTestEnable = true;
		depthStencilDesc.depthWriteEnable = false;
		depthStencilDesc.depthCompareOp = COMPARE_OP::LESS_OR_EQUAL;
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
		pipelineDesc.vertexShader = m_modelVisibilityVertShader.get();
		pipelineDesc.fragmentShader = m_modelVisibilityFragShader.get();
		pipelineDesc.rootSignature = m_modelVisibilityPassRootSignature.get();
		pipelineDesc.vertexInputDesc = vertexInputDesc;
		pipelineDesc.inputAssemblyDesc = inputAssemblyDesc;
		pipelineDesc.rasteriserDesc = rasteriserDesc;
		pipelineDesc.depthStencilDesc = depthStencilDesc;
		pipelineDesc.multisamplingDesc = multisamplingDesc;
		pipelineDesc.colourBlendDesc = colourBlendDesc;
		pipelineDesc.colourAttachmentFormats = {};
		pipelineDesc.depthStencilAttachmentFormat = DATA_FORMAT::D32_SFLOAT;

		m_modelVisibilityPipeline = m_device->CreatePipeline(pipelineDesc);
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
		pipelineDesc.rootSignature = m_shadowPassRootSignature.get();
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
		pipelineDesc.colourAttachmentFormats = { DATA_FORMAT::R16G16B16A16_SFLOAT };
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
		pipelineDesc.colourAttachmentFormats = { DATA_FORMAT::R16G16B16A16_SFLOAT };
		pipelineDesc.depthStencilAttachmentFormat = DATA_FORMAT::D32_SFLOAT;

		m_blinnPhongPipeline = m_device->CreatePipeline(pipelineDesc);

		pipelineDesc.fragmentShader = m_pbrFragShader.get();
		m_pbrPipeline = m_device->CreatePipeline(pipelineDesc);
	}



	void RenderLayer::InitPostprocessPipeline()
	{
		std::vector<VertexAttributeDesc> vertexAttributes;
		VertexAttributeDesc posAttribute{};
		posAttribute.attribute = SHADER_ATTRIBUTE::POSITION;
		posAttribute.binding = 0;
		posAttribute.format = DATA_FORMAT::R32G32B32_SFLOAT;
		posAttribute.offset = 0;
		vertexAttributes.push_back(posAttribute);
		VertexAttributeDesc uvAttribute{};
		uvAttribute.attribute = SHADER_ATTRIBUTE::TEXCOORD_0;
		uvAttribute.binding = 0;
		uvAttribute.format = DATA_FORMAT::R32G32_SFLOAT;
		uvAttribute.offset = sizeof(glm::vec3);
		vertexAttributes.push_back(uvAttribute);
		std::vector<VertexBufferBindingDesc> bufferBindings;
		VertexBufferBindingDesc bufferBinding{};
		bufferBinding.binding = 0;
		bufferBinding.inputRate = VERTEX_INPUT_RATE::VERTEX;
		bufferBinding.stride = sizeof(ScreenQuadVertex);
		bufferBindings.push_back(bufferBinding);
		VertexInputDesc vertexInputDesc{};
		vertexInputDesc.attributeDescriptions = vertexAttributes;
		vertexInputDesc.bufferBindingDescriptions = bufferBindings;

		InputAssemblyDesc inputAssemblyDesc{};
		inputAssemblyDesc.topology = INPUT_TOPOLOGY::TRIANGLE_LIST;

		RasteriserDesc rasteriserDesc{};
		rasteriserDesc.cullMode = CULL_MODE::BACK;
		rasteriserDesc.frontFace = WINDING_DIRECTION::CLOCKWISE;
		rasteriserDesc.depthBiasEnable = false;

		DepthStencilDesc depthStencilDesc{};
		depthStencilDesc.depthTestEnable = true;
		depthStencilDesc.depthWriteEnable = true;
		depthStencilDesc.depthCompareOp = COMPARE_OP::LESS_OR_EQUAL;
		depthStencilDesc.stencilTestEnable = false;

		MultisamplingDesc multisamplingDesc{};
		multisamplingDesc.sampleCount = SAMPLE_COUNT::BIT_1;
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
		pipelineDesc.vertexShader = m_screenQuadVertShader.get();
		pipelineDesc.fragmentShader = m_postprocessFragShader.get();
		pipelineDesc.rootSignature = m_postprocessPassRootSignature.get();
		pipelineDesc.vertexInputDesc = vertexInputDesc;
		pipelineDesc.inputAssemblyDesc = inputAssemblyDesc;
		pipelineDesc.rasteriserDesc = rasteriserDesc;
		pipelineDesc.depthStencilDesc = depthStencilDesc;
		pipelineDesc.multisamplingDesc = multisamplingDesc;
		pipelineDesc.colourBlendDesc = colourBlendDesc;
		pipelineDesc.colourAttachmentFormats = { DATA_FORMAT::R8G8B8A8_SRGB };
		pipelineDesc.depthStencilAttachmentFormat = DATA_FORMAT::D32_SFLOAT;

		m_postprocessPipeline = m_device->CreatePipeline(pipelineDesc);
	}



	void RenderLayer::InitRenderGraphs()
	{
		RenderGraphDesc meshDesc{};


		
		meshDesc.AddNode(
		"MODEL_VISIBILITY_PASS",
		{{ "CAMERA_BUFFER_PREVIOUS_FRAME", RESOURCE_STATE::CONSTANT_BUFFER },
		{ "MODEL_MATRICES_BUFFER", RESOURCE_STATE::SHADER_RESOURCE },
		{ "MODEL_VISIBILITY_BUFFER", RESOURCE_STATE::UNORDERED_ACCESS },
		{ "SCENE_DEPTH", RESOURCE_STATE::DEPTH_WRITE },
		{ "SCENE_DEPTH_MSAA", RESOURCE_STATE::DEPTH_WRITE },
		{ "SCENE_DEPTH_SSAA", RESOURCE_STATE::DEPTH_WRITE }},
		[&](ICommandBuffer* _cmdBuf, const BindingMap<IBuffer>& _bufs, const BindingMap<ITexture>& _texs, const BindingMap<IBufferView>& _bufViews, const BindingMap<ITextureView>& _texViews, const BindingMap<ISampler>& _samplers)
		{
			if (m_desc.enableMSAA)
			{
				_cmdBuf->BeginRendering(0, nullptr, nullptr, _texViews.Get("SCENE_DEPTH_MSAA_DSV"), _texViews.Get("SCENE_DEPTH_DSV"), nullptr, true, false);
			}
			else if (m_desc.enableSSAA)
			{
				_cmdBuf->BeginRendering(0, nullptr, nullptr, nullptr, _texViews.Get("SCENE_DEPTH_SSAA_DSV"), nullptr, true, false);
			}
			else
			{
				_cmdBuf->BeginRendering(0, nullptr, nullptr, nullptr, _texViews.Get("SCENE_DEPTH_DSV"), nullptr, true, false);
			}

			_cmdBuf->BindRootSignature(m_modelVisibilityPassRootSignature.get(), PIPELINE_BIND_POINT::GRAPHICS);

			_cmdBuf->SetViewport({ 0, 0 }, { m_desc.enableSSAA ? m_supersampleResolution : m_desc.window->GetSize() });
			_cmdBuf->SetScissor({ 0, 0 }, { m_desc.enableSSAA ? m_supersampleResolution : m_desc.window->GetSize() });

			ModelVisibilityPassPushConstantData pushConstantData{};
			pushConstantData.camDataBufferIndex = _bufViews.Get("CAMERA_BUFFER_PREVIOUS_FRAME_VIEW")->GetIndex();
			pushConstantData.modelMatricesBufferIndex = _bufViews.Get("MODEL_MATRICES_BUFFER_VIEW")->GetIndex();
			pushConstantData.modelVisibilityBufferIndex = _bufViews.Get("MODEL_VISIBILITY_BUFFER_VIEW")->GetIndex();
							
			//Models
			std::size_t cubeVertexBufferStride{ sizeof(glm::vec3) };
			_cmdBuf->PushConstants(m_modelVisibilityPassRootSignature.get(), &pushConstantData);
			_cmdBuf->BindPipeline(m_modelVisibilityPipeline.get(), PIPELINE_BIND_POINT::GRAPHICS);
			_cmdBuf->BindVertexBuffers(0, 1, m_cubeVertBuffer.get(), &cubeVertexBufferStride);
			_cmdBuf->BindIndexBuffer(m_cubeIndexBuffer.get(), DATA_FORMAT::R32_UINT);
			_cmdBuf->DrawIndexed(36, m_modelMatrices.size(), 0, 0);
			_cmdBuf->EndRendering(0, nullptr, nullptr);
		});



		meshDesc.AddNode(
		"CAMERA_BUFFER_PREVIOUS_FRAME_COPY_PASS",
		{{ "CAMERA_BUFFER", RESOURCE_STATE::COPY_SOURCE },
		{ "CAMERA_BUFFER_PREVIOUS_FRAME", RESOURCE_STATE::COPY_DEST }},
		[&](ICommandBuffer* _cmdBuf, const BindingMap<IBuffer>& _bufs, const BindingMap<ITexture>& _texs, const BindingMap<IBufferView>& _bufViews, const BindingMap<ITextureView>& _texViews, const BindingMap<ISampler>& _samplers)
		{
			_cmdBuf->CopyBufferToBuffer(_bufs.Get("CAMERA_BUFFER"), _bufs.Get("CAMERA_BUFFER_PREVIOUS_FRAME"), 0, 0, sizeof(CameraShaderData));
		}, true);


		
		meshDesc.AddNode(
		"DEPTH_BARRIER",
		{{ "SCENE_DEPTH", RESOURCE_STATE::DEPTH_READ },
		 { "SCENE_DEPTH_MSAA", RESOURCE_STATE::DEPTH_READ },
		 { "SCENE_DEPTH_SSAA", RESOURCE_STATE::DEPTH_READ }}
		);


		
		meshDesc.AddNode(
		"SHADOW_PASS",
		{{ "LIGHT_DATA_BUFFER", RESOURCE_STATE::SHADER_RESOURCE }},
		[&](ICommandBuffer* _cmdBuf, const BindingMap<IBuffer>& _bufs, const BindingMap<ITexture>& _texs, const BindingMap<IBufferView>& _bufViews, const BindingMap<ITextureView>& _texViews, const BindingMap<ISampler>& _samplers)
		{
			_cmdBuf->BindPipeline(m_shadowPipeline.get(), PIPELINE_BIND_POINT::GRAPHICS);
			_cmdBuf->BindRootSignature(m_shadowPassRootSignature.get(), PIPELINE_BIND_POINT::GRAPHICS);

			std::size_t modelVertexBufferStride{ sizeof(ModelVertex) };
			
			ShadowPassPushConstantData pushConstantData{};

			
			auto drawModels{ [&]()
			{
				for (auto&& [modelRenderer, transform] : m_reg.get().View<CModelRenderer, CTransform>())
				{
					if (!modelRenderer.visible) { continue; }
					const GPUModel* const model{ modelRenderer.model };
					if (!model) { continue; }
					pushConstantData.modelMat = transform.GetModelMatrix();

					for (std::size_t meshIndex{ 0 }; meshIndex < modelRenderer.model->meshes.size(); ++meshIndex)
					{
						const GPUMesh* mesh{ model->meshes[meshIndex].get() };
						_cmdBuf->PushConstants(m_shadowPassRootSignature.get(), &pushConstantData);
						_cmdBuf->BindVertexBuffers(0, 1, mesh->vertexBuffer.get(), &modelVertexBufferStride);
						_cmdBuf->BindIndexBuffer(mesh->indexBuffer.get(), DATA_FORMAT::R32_UINT);
						_cmdBuf->DrawIndexed(mesh->indexCount, 1, 0, 0);
					}
				}
			} };

			
			//Loop through all lights
			//View is parallel to vector
			//^todo: there's almost definitely a better way of handling this
			ComponentView<CLight, CTransform> lightView{ m_reg.get().View<CLight, CTransform>() };
			ComponentView<CLight, CTransform>::iterator lightIt{ lightView.begin() };
			for (std::size_t i{ 0 }; i < m_cpuLightData.size(); ++i, ++lightIt)
			{
				auto [light, transform] = *lightIt;
				ITexture* shadowMap{ nullptr };
				const LightShaderData& lightData{ m_cpuLightData[i] };
				if (lightData.type == LIGHT_TYPE::POINT)
				{
					shadowMap = m_shadowMapsCube[light.light->GetShadowMapVectorIndex()].get();
					_cmdBuf->TransitionBarrier(shadowMap, shadowMap->GetState(), RESOURCE_STATE::DEPTH_WRITE);
					
					//shadow cubemap - need to render scene 6 times from all faces and set view matrix between each one
					for (std::uint32_t faceIndex{ 0 }; faceIndex < 6; ++faceIndex)
					{
						pushConstantData.viewProjMat = m_pointLightProjMatrix * GetPointLightViewMatrix(lightData.position, faceIndex);
						_cmdBuf->BeginRendering(0, nullptr, nullptr, nullptr, m_shadowMapCube_FaceDSVs[light.light->GetShadowMapVectorIndex()][faceIndex].get(), nullptr, true, true);
						_cmdBuf->SetViewport({ 0, 0 }, { shadowMap->GetSize().x, shadowMap->GetSize().y });
						_cmdBuf->SetScissor({ 0, 0 }, { shadowMap->GetSize().x, shadowMap->GetSize().y });
						drawModels();
						_cmdBuf->EndRendering(0, nullptr, nullptr);
					}
				}
				else
				{
					shadowMap = m_shadowMaps2D[light.light->GetShadowMapVectorIndex()].get();
					_cmdBuf->TransitionBarrier(shadowMap, shadowMap->GetState(), RESOURCE_STATE::DEPTH_WRITE);
					
					//2d shadow map
					pushConstantData.viewProjMat = lightData.viewProjMat; //todo: this is so ugly, why is it being duplicated? find a better way of doing this !!
					_cmdBuf->BeginRendering(0, nullptr, nullptr, nullptr, m_shadowMap2DDSVs[light.light->GetShadowMapVectorIndex()].get(), nullptr, true, true);
					_cmdBuf->SetViewport({ 0, 0 }, { shadowMap->GetSize().x, shadowMap->GetSize().y });
					_cmdBuf->SetScissor({ 0, 0 }, { shadowMap->GetSize().x, shadowMap->GetSize().y });
					drawModels();
					_cmdBuf->EndRendering(0, nullptr, nullptr);
				}

				_cmdBuf->TransitionBarrier(shadowMap, RESOURCE_STATE::DEPTH_WRITE, RESOURCE_STATE::SHADER_RESOURCE); //prepare for next pass
			}
			
		}, true);


		meshDesc.AddNode(
		"SCENE_PASS",
		{{ "CAMERA_BUFFER", RESOURCE_STATE::CONSTANT_BUFFER },
		{ "LIGHT_DATA_BUFFER", RESOURCE_STATE::CONSTANT_BUFFER },
		{ "SCENE_COLOUR", RESOURCE_STATE::RENDER_TARGET },
		{ "SCENE_COLOUR_MSAA", RESOURCE_STATE::RENDER_TARGET },
		{ "SCENE_COLOUR_SSAA", RESOURCE_STATE::RENDER_TARGET },
		{ "SCENE_DEPTH", RESOURCE_STATE::DEPTH_WRITE },
		{ "SCENE_DEPTH_MSAA", RESOURCE_STATE::DEPTH_WRITE },
		{ "SCENE_DEPTH_SSAA", RESOURCE_STATE::DEPTH_WRITE },
		{ "SKYBOX", RESOURCE_STATE::SHADER_RESOURCE },
		{ "IRRADIANCE_MAP", RESOURCE_STATE::SHADER_RESOURCE },
		{ "PREFILTER_MAP", RESOURCE_STATE::SHADER_RESOURCE },
		{ "BRDF_LUT", RESOURCE_STATE::SHADER_RESOURCE }},
		[&](ICommandBuffer* _cmdBuf, const BindingMap<IBuffer>& _bufs, const BindingMap<ITexture>& _texs, const BindingMap<IBufferView>& _bufViews, const BindingMap<ITextureView>& _texViews, const BindingMap<ISampler>& _samplers)
		{
			if (m_desc.enableMSAA)
			{
				_cmdBuf->BeginRendering(1, nullptr, _texViews.Get("SCENE_COLOUR_MSAA_RTV"), _texViews.Get("SCENE_DEPTH_MSAA_DSV"), _texViews.Get("SCENE_DEPTH_DSV"), nullptr);
			}
			else if (m_desc.enableSSAA)
			{
				_cmdBuf->BeginRendering(1, nullptr, _texViews.Get("SCENE_COLOUR_SSAA_RTV"), nullptr, _texViews.Get("SCENE_DEPTH_SSAA_DSV"), nullptr);
			}
			else
			{
				_cmdBuf->BeginRendering(1, nullptr, _texViews.Get("SCENE_COLOUR_RTV"), nullptr, _texViews.Get("SCENE_DEPTH_DSV"), nullptr);
			}
			
			_cmdBuf->BindRootSignature(m_meshPassRootSignature.get(), PIPELINE_BIND_POINT::GRAPHICS);

			_cmdBuf->SetViewport({ 0, 0 }, { m_desc.enableSSAA ? m_supersampleResolution : m_desc.window->GetSize() });
			_cmdBuf->SetScissor({ 0, 0 }, { m_desc.enableSSAA ? m_supersampleResolution : m_desc.window->GetSize() });

			MeshPassPushConstantData pushConstantData{};
			pushConstantData.camDataBufferIndex = _bufViews.Get("CAMERA_BUFFER_VIEW")->GetIndex();
			pushConstantData.numLights = m_cpuLightData.size();
			pushConstantData.lightDataBufferIndex = _bufViews.Get("LIGHT_DATA_BUFFER_VIEW")->GetIndex();
			pushConstantData.skyboxCubemapIndex = _texViews.Get("SKYBOX_VIEW") ? _texViews.Get("SKYBOX_VIEW")->GetIndex() : 0;
			pushConstantData.irradianceCubemapIndex = _texViews.Get("IRRADIANCE_MAP_VIEW") ? _texViews.Get("IRRADIANCE_MAP_VIEW")->GetIndex() : 0;
			pushConstantData.prefilterCubemapIndex = _texViews.Get("PREFILTER_MAP_VIEW") ? _texViews.Get("PREFILTER_MAP_VIEW")->GetIndex() : 0;
			pushConstantData.brdfLUTIndex = _texViews.Get("BRDF_LUT_VIEW")->GetIndex();
			pushConstantData.brdfLUTSamplerIndex = _samplers.Get("BRDF_LUT_SAMPLER")->GetIndex();
			pushConstantData.samplerIndex = _samplers.Get("SAMPLER")->GetIndex();

			//Skybox
			if (_texs.Get("SKYBOX"))
			{
				std::size_t skyboxVertexBufferStride{ sizeof(glm::vec3) };
				_cmdBuf->PushConstants(m_meshPassRootSignature.get(), &pushConstantData);
				_cmdBuf->BindPipeline(m_skyboxPipeline.get(), PIPELINE_BIND_POINT::GRAPHICS);
				_cmdBuf->BindVertexBuffers(0, 1, m_cubeVertBuffer.get(), &skyboxVertexBufferStride);
				_cmdBuf->BindIndexBuffer(m_cubeIndexBuffer.get(), DATA_FORMAT::R32_UINT);
				_cmdBuf->DrawIndexed(36, 1, 0, 0);
			}

			//Models
			std::size_t modelVertexBufferStride{ sizeof(ModelVertex) };
			for (auto&& [modelRenderer, transform] : m_reg.get().View<CModelRenderer, CTransform>())
			{
				if (!modelRenderer.visible) { continue; }
				const GPUModel* const model{ modelRenderer.model };
				if (!model) { continue; }
				pushConstantData.modelMat = transform.GetModelMatrix();

				for (std::size_t i{ 0 }; i < modelRenderer.model->meshes.size(); ++i)
				{
					const GPUMesh* mesh{ model->meshes[i].get() };
					pushConstantData.materialBufferIndex = model->materials[mesh->materialIndex]->bufferIndex;
					_cmdBuf->PushConstants(m_meshPassRootSignature.get(), &pushConstantData);
					IPipeline* pipeline{ model->materials[mesh->materialIndex]->lightingModel == LIGHTING_MODEL::BLINN_PHONG ? m_blinnPhongPipeline.get() : m_pbrPipeline.get() };
					_cmdBuf->BindPipeline(pipeline, PIPELINE_BIND_POINT::GRAPHICS);
					_cmdBuf->BindVertexBuffers(0, 1, mesh->vertexBuffer.get(), &modelVertexBufferStride);
					_cmdBuf->BindIndexBuffer(mesh->indexBuffer.get(), DATA_FORMAT::R32_UINT);
					_cmdBuf->DrawIndexed(mesh->indexCount, 1, 0, 0);
				}
			}

			if (m_desc.enableMSAA)
			{
				_cmdBuf->EndRendering(1, nullptr, _texs.Get("SCENE_COLOUR_MSAA"));
			}
			else if (m_desc.enableSSAA)
			{
				_cmdBuf->EndRendering(1, nullptr, _texs.Get("SCENE_COLOUR_SSAA"));
			}
			else
			{
				_cmdBuf->EndRendering(1, nullptr, _texs.Get("SCENE_COLOUR"));
			}
		});
		
		if (m_desc.enableSSAA)
		{
			meshDesc.AddNode(
			"SSAA_DOWNSAMPLE_PASS",
			{{"SCENE_COLOUR_SSAA", RESOURCE_STATE::COPY_SOURCE},
			{"SCENE_COLOUR", RESOURCE_STATE::COPY_DEST},
			{"SCENE_DEPTH_SSAA", RESOURCE_STATE::COPY_SOURCE},
			{"SCENE_DEPTH", RESOURCE_STATE::COPY_DEST}},
			[&](ICommandBuffer* _cmdBuf, const BindingMap<IBuffer>& _bufs, const BindingMap<ITexture>& _texs, const BindingMap<IBufferView>& _bufViews, const BindingMap<ITextureView>& _texViews, const BindingMap<ISampler>& _samplers)
			{
				_cmdBuf->BlitTexture(_texs.Get("SCENE_COLOUR_SSAA"), TEXTURE_ASPECT::COLOUR, _texs.Get("SCENE_COLOUR"), TEXTURE_ASPECT::COLOUR);
				_cmdBuf->BlitTexture(_texs.Get("SCENE_DEPTH_SSAA"), TEXTURE_ASPECT::DEPTH, _texs.Get("SCENE_DEPTH"), TEXTURE_ASPECT::DEPTH);
			});
		}

		//If MSAA is enabled, SHADOW_MAP_MSAA, SCENE_COLOUR_MSAA, and SCENE_DEPTH_MSAA need to be resolved into SHADOW_MAP, SCENE_COLOUR, and SCENE_DEPTH before being sampled in the postprocess shader
		if (m_desc.enableMSAA)
		{
			meshDesc.AddNode(
			"MSAA_RESOLVE_PASS",
			{{"SCENE_COLOUR", RESOURCE_STATE::COPY_DEST},
			{"SCENE_COLOUR_MSAA", RESOURCE_STATE::COPY_SOURCE},
			{"SCENE_DEPTH", RESOURCE_STATE::COPY_DEST},
			{"SCENE_DEPTH_MSAA", RESOURCE_STATE::COPY_SOURCE}},
			[&](ICommandBuffer* _cmdBuf, const BindingMap<IBuffer>& _bufs, const BindingMap<ITexture>& _texs, const BindingMap<IBufferView>& _bufViews, const BindingMap<ITextureView>& _texViews, const BindingMap<ISampler>& _samplers)
			{
				//todo fix this
				//_cmdBuf->ResolveImage(_texs.Get("SCENE_DEPTH_MSAA"), _texs.Get("SCENE_DEPTH"), TEXTURE_ASPECT::DEPTH);

				_cmdBuf->ResolveImage(_texs.Get("SCENE_COLOUR_MSAA"), _texs.Get("SCENE_COLOUR"), TEXTURE_ASPECT::COLOUR);
			});
		}
		
		meshDesc.AddNode(
		"POSTPROCESS_PASS",
		{{ "SCENE_COLOUR", RESOURCE_STATE::SHADER_RESOURCE },
		{ "SCENE_COLOUR_SSAA", RESOURCE_STATE::SHADER_RESOURCE },
		{ "SCENE_DEPTH", RESOURCE_STATE::SHADER_RESOURCE },
		{ "SCENE_DEPTH_SSAA", RESOURCE_STATE::SHADER_RESOURCE },
		{ "BACKBUFFER", RESOURCE_STATE::RENDER_TARGET }},
		[&](ICommandBuffer* _cmdBuf, const BindingMap<IBuffer>& _bufs, const BindingMap<ITexture>& _texs, const BindingMap<IBufferView>& _bufViews, const BindingMap<ITextureView>& _texViews, const BindingMap<ISampler>& _samplers)
		{
			_cmdBuf->BeginRendering(1, nullptr, _texViews.Get("BACKBUFFER_RTV"), nullptr, nullptr, nullptr);
			_cmdBuf->BindRootSignature(m_postprocessPassRootSignature.get(), PIPELINE_BIND_POINT::GRAPHICS);

			_cmdBuf->SetViewport({ 0, 0 }, { m_desc.window->GetSize() });
			_cmdBuf->SetScissor({ 0, 0 }, { m_desc.window->GetSize() });

			PostprocessPassPushConstantData pushConstantData{};
			pushConstantData.sceneColourIndex = _texViews.Get("SCENE_COLOUR_SRV")->GetIndex();
			pushConstantData.sceneDepthIndex = _texViews.Get("SCENE_DEPTH_SRV")->GetIndex();
			pushConstantData.samplerIndex = _samplers.Get("SAMPLER")->GetIndex();

			//Screen Quad
			std::size_t screenQuadVertexBufferStride{ sizeof(ScreenQuadVertex) };
			_cmdBuf->PushConstants(m_postprocessPassRootSignature.get(), &pushConstantData);
			_cmdBuf->BindPipeline(m_postprocessPipeline.get(), PIPELINE_BIND_POINT::GRAPHICS);
			_cmdBuf->BindVertexBuffers(0, 1, m_screenQuadVertBuffer.get(), &screenQuadVertexBufferStride);
			_cmdBuf->BindIndexBuffer(m_screenQuadIndexBuffer.get(), DATA_FORMAT::R32_UINT);
			_cmdBuf->DrawIndexed(6, 1, 0, 0);

			_cmdBuf->EndRendering(1, nullptr, _texs.Get("BACKBUFFER"));
		});

		
		//Transition backbuffer to present state
		meshDesc.AddNode("PRESENT_TRANSITION_PASS", {{"BACKBUFFER", RESOURCE_STATE::PRESENT}});
		
		
		m_meshRenderGraph = UniquePtr<RenderGraph>(NK_NEW(RenderGraph, meshDesc));
	}



	void RenderLayer::InitScreenResources()
	{
		//--------SHADOW MAPS--------//
		

		
		//--------END OF SHADOW MAPS--------//
		
		
		//--------SCENE COLOUR--------//
		
		//Scene Colour
		TextureDesc sceneColourDesc{};
		sceneColourDesc.dimension = TEXTURE_DIMENSION::DIM_2;
		sceneColourDesc.format = DATA_FORMAT::R16G16B16A16_SFLOAT;
		sceneColourDesc.size = glm::ivec3(m_desc.window->GetSize(), 1);
		sceneColourDesc.usage = TEXTURE_USAGE_FLAGS::COLOUR_ATTACHMENT | TEXTURE_USAGE_FLAGS::READ_ONLY | TEXTURE_USAGE_FLAGS::TRANSFER_DST_BIT;
		sceneColourDesc.arrayTexture = false;
		sceneColourDesc.sampleCount = SAMPLE_COUNT::BIT_1;
		m_sceneColour = m_device->CreateTexture(sceneColourDesc);
		m_graphicsCommandBuffers[0]->TransitionBarrier(m_sceneColour.get(), RESOURCE_STATE::UNDEFINED, RESOURCE_STATE::RENDER_TARGET);
		
		//MSAA Scene Colour
		if (m_desc.enableMSAA)
		{
			sceneColourDesc.sampleCount = m_desc.msaaSampleCount;
			sceneColourDesc.usage |= TEXTURE_USAGE_FLAGS::TRANSFER_SRC_BIT;
			m_sceneColourMSAA = m_device->CreateTexture(sceneColourDesc);
			m_graphicsCommandBuffers[0]->TransitionBarrier(m_sceneColourMSAA.get(), RESOURCE_STATE::UNDEFINED, RESOURCE_STATE::RENDER_TARGET);
		}
		
		//SSAA Scene Colour
		else if (m_desc.enableSSAA)
		{
			sceneColourDesc.size = glm::ivec3(m_supersampleResolution, 1);
			sceneColourDesc.usage |= TEXTURE_USAGE_FLAGS::TRANSFER_SRC_BIT;
			m_sceneColourSSAA = m_device->CreateTexture(sceneColourDesc);
			m_graphicsCommandBuffers[0]->TransitionBarrier(m_sceneColourSSAA.get(), RESOURCE_STATE::UNDEFINED, RESOURCE_STATE::RENDER_TARGET);
		}
		
		//Scene Colour RTVs
		TextureViewDesc sceneColourViewDesc{};
		sceneColourViewDesc.dimension = TEXTURE_VIEW_DIMENSION::DIM_2;
		sceneColourViewDesc.format = DATA_FORMAT::R16G16B16A16_SFLOAT;
		sceneColourViewDesc.type = TEXTURE_VIEW_TYPE::RENDER_TARGET;
		m_sceneColourRTV = m_device->CreateRenderTargetTextureView(m_sceneColour.get(), sceneColourViewDesc);
		if (m_desc.enableMSAA) { m_sceneColourMSAARTV = m_device->CreateRenderTargetTextureView(m_sceneColourMSAA.get(), sceneColourViewDesc); }
		if (m_desc.enableSSAA) { m_sceneColourSSAARTV = m_device->CreateRenderTargetTextureView(m_sceneColourSSAA.get(), sceneColourViewDesc); }
		
		//Scene Colour SRVs
		sceneColourViewDesc.type = TEXTURE_VIEW_TYPE::SHADER_READ_ONLY;
		m_sceneColourSRV = m_device->CreateShaderResourceTextureView(m_sceneColour.get(), sceneColourViewDesc);
		if (m_desc.enableMSAA) { m_sceneColourMSAASRV = m_device->CreateShaderResourceTextureView(m_sceneColourMSAA.get(), sceneColourViewDesc); }
		if (m_desc.enableSSAA) { m_sceneColourSSAASRV = m_device->CreateShaderResourceTextureView(m_sceneColourSSAA.get(), sceneColourViewDesc); }
		
		//--------END OF SCENE COLOUR--------//
		

		//--------SCENE DEPTH--------//
		
		//Scene Depth
		TextureDesc sceneDepthDesc{};
		sceneDepthDesc.dimension = TEXTURE_DIMENSION::DIM_2;
		sceneDepthDesc.format = DATA_FORMAT::D32_SFLOAT;
		sceneDepthDesc.size = glm::ivec3(m_desc.window->GetSize(), 1);
		sceneDepthDesc.usage = TEXTURE_USAGE_FLAGS::DEPTH_STENCIL_ATTACHMENT | TEXTURE_USAGE_FLAGS::READ_ONLY | TEXTURE_USAGE_FLAGS::TRANSFER_DST_BIT;
		sceneDepthDesc.arrayTexture = false;
		sceneDepthDesc.sampleCount = SAMPLE_COUNT::BIT_1;
		m_sceneDepth = m_device->CreateTexture(sceneDepthDesc);
		m_graphicsCommandBuffers[0]->TransitionBarrier(m_sceneDepth.get(), RESOURCE_STATE::UNDEFINED, RESOURCE_STATE::DEPTH_WRITE);

		//MSAA Scene Depth
		if (m_desc.enableMSAA)
		{
			sceneDepthDesc.sampleCount = m_desc.msaaSampleCount;
			sceneDepthDesc.usage |= TEXTURE_USAGE_FLAGS::TRANSFER_SRC_BIT;
			m_sceneDepthMSAA = m_device->CreateTexture(sceneDepthDesc);
			m_graphicsCommandBuffers[0]->TransitionBarrier(m_sceneDepthMSAA.get(), RESOURCE_STATE::UNDEFINED, RESOURCE_STATE::DEPTH_WRITE);
		}
		
		//SSAA Scene Depth
		if (m_desc.enableSSAA)
		{
			sceneDepthDesc.size = glm::ivec3(m_supersampleResolution, 1);
			sceneDepthDesc.usage |= TEXTURE_USAGE_FLAGS::TRANSFER_SRC_BIT;
			m_sceneDepthSSAA = m_device->CreateTexture(sceneDepthDesc);
			m_graphicsCommandBuffers[0]->TransitionBarrier(m_sceneDepthSSAA.get(), RESOURCE_STATE::UNDEFINED, RESOURCE_STATE::DEPTH_WRITE);
		}
		
		//Scene Depth DSVs
		TextureViewDesc sceneDepthViewDesc{};
		sceneDepthViewDesc.dimension = TEXTURE_VIEW_DIMENSION::DIM_2;
		sceneDepthViewDesc.format = DATA_FORMAT::D32_SFLOAT;
		sceneDepthViewDesc.type = TEXTURE_VIEW_TYPE::DEPTH;
		m_sceneDepthDSV = m_device->CreateDepthStencilTextureView(m_sceneDepth.get(), sceneDepthViewDesc);
		if (m_desc.enableMSAA) { m_sceneDepthMSAADSV = m_device->CreateDepthStencilTextureView(m_sceneDepthMSAA.get(), sceneDepthViewDesc); }
		if (m_desc.enableSSAA) { m_sceneDepthSSAADSV = m_device->CreateDepthStencilTextureView(m_sceneDepthSSAA.get(), sceneDepthViewDesc); }
		
		//Shadow Map SRVs
		sceneDepthViewDesc.type = TEXTURE_VIEW_TYPE::SHADER_READ_ONLY;
		m_sceneDepthSRV = m_device->CreateShaderResourceTextureView(m_sceneDepth.get(), sceneDepthViewDesc);
		if (m_desc.enableMSAA) { m_sceneDepthMSAASRV = m_device->CreateShaderResourceTextureView(m_sceneDepthMSAA.get(), sceneDepthViewDesc); }
		if (m_desc.enableSSAA) { m_sceneDepthSSAASRV = m_device->CreateShaderResourceTextureView(m_sceneDepthSSAA.get(), sceneDepthViewDesc); }
		
		//--------END OF SCENE DEPTH--------//
	}



	void RenderLayer::InitBRDFLut()
	{
		ImageData* brdfLutImageData{ ImageLoader::LoadImage("Samples/Resource-Files/Skyboxes/brdf_lut.png", true, false) };
		brdfLutImageData->desc.usage |= TEXTURE_USAGE_FLAGS::READ_ONLY;
		m_brdfLUT = m_device->CreateTexture(brdfLutImageData->desc);
		m_gpuUploader->EnqueueTextureDataUpload(brdfLutImageData->data, m_brdfLUT.get(), RESOURCE_STATE::UNDEFINED);

		TextureViewDesc brdfLutTextureViewDesc{};
		brdfLutTextureViewDesc.dimension = TEXTURE_VIEW_DIMENSION::DIM_2;
		brdfLutTextureViewDesc.format = DATA_FORMAT::R8G8B8A8_UNORM;
		brdfLutTextureViewDesc.type = TEXTURE_VIEW_TYPE::SHADER_READ_ONLY;
		m_brdfLUTView = m_device->CreateShaderResourceTextureView(m_brdfLUT.get(), brdfLutTextureViewDesc);
	}



	void RenderLayer::InitShadowMapForLight(const CLight& _light)
	{
		//Shadow Map
		TextureDesc shadowMapDesc{};
		shadowMapDesc.dimension = TEXTURE_DIMENSION::DIM_2;
		shadowMapDesc.format = DATA_FORMAT::D32_SFLOAT;
		shadowMapDesc.usage = TEXTURE_USAGE_FLAGS::DEPTH_STENCIL_ATTACHMENT | TEXTURE_USAGE_FLAGS::READ_ONLY | TEXTURE_USAGE_FLAGS::TRANSFER_DST_BIT;
		shadowMapDesc.sampleCount = SAMPLE_COUNT::BIT_1;
		if (_light.lightType == LIGHT_TYPE::POINT)
		{
			shadowMapDesc.size = glm::ivec3(m_shadowMapBaseResolution * glm::ivec2(m_desc.enableSSAA ? m_desc.ssaaMultiplier : 1), 6);
			shadowMapDesc.arrayTexture = true;
			shadowMapDesc.cubemap = true;
			m_shadowMapsCube.push_back(m_device->CreateTexture(shadowMapDesc));
			m_graphicsCommandBuffers[m_currentFrame]->TransitionBarrier(m_shadowMapsCube[m_shadowMapsCube.size() - 1].get(), RESOURCE_STATE::UNDEFINED, RESOURCE_STATE::DEPTH_WRITE);
		}
		else
		{
			shadowMapDesc.size = glm::ivec3(m_shadowMapBaseResolution * glm::ivec2(m_desc.enableSSAA ? m_desc.ssaaMultiplier : 1), 1);
			m_shadowMaps2D.push_back(m_device->CreateTexture(shadowMapDesc));
			m_graphicsCommandBuffers[m_currentFrame]->TransitionBarrier(m_shadowMaps2D[m_shadowMaps2D.size() - 1].get(), RESOURCE_STATE::UNDEFINED, RESOURCE_STATE::DEPTH_WRITE);
		}

		
		//DSV(s) and SRV
		TextureViewDesc shadowMapViewDesc{};
		shadowMapViewDesc.format = DATA_FORMAT::D32_SFLOAT;
		shadowMapViewDesc.type = TEXTURE_VIEW_TYPE::DEPTH;
		if (_light.lightType == LIGHT_TYPE::POINT)
		{
			shadowMapViewDesc.dimension = TEXTURE_VIEW_DIMENSION::DIM_2D_ARRAY;
			
			//DSVs
			shadowMapViewDesc.arrayLayerCount = 1;
			m_shadowMapCube_FaceDSVs.push_back({});
			m_shadowMapCube_FaceDSVs[m_shadowMapCube_FaceDSVs.size() - 1].resize(6);
			for (std::size_t i{ 0 }; i < 6; ++i)
			{
				shadowMapViewDesc.baseArrayLayer = i;
				m_shadowMapCube_FaceDSVs[m_shadowMapCube_FaceDSVs.size() - 1][i] = m_device->CreateDepthStencilTextureView(m_shadowMapsCube[m_shadowMapsCube.size() - 1].get(), shadowMapViewDesc);
			}

			//SRV
			shadowMapViewDesc.baseArrayLayer = 0;
			shadowMapViewDesc.arrayLayerCount = 6;
			shadowMapViewDesc.type = TEXTURE_VIEW_TYPE::SHADER_READ_ONLY;
			shadowMapViewDesc.dimension = TEXTURE_VIEW_DIMENSION::DIM_CUBE;
			m_shadowMapCubeSRVs.push_back(m_device->CreateShaderResourceTextureView(m_shadowMapsCube[m_shadowMapsCube.size() - 1].get(), shadowMapViewDesc));
		}
		else
		{
			shadowMapViewDesc.dimension = TEXTURE_VIEW_DIMENSION::DIM_2;
			m_shadowMap2DDSVs.push_back(m_device->CreateDepthStencilTextureView(m_shadowMaps2D[m_shadowMaps2D.size() - 1].get(), shadowMapViewDesc));
			shadowMapViewDesc.type = TEXTURE_VIEW_TYPE::SHADER_READ_ONLY;
			m_shadowMap2DSRVs.push_back(m_device->CreateShaderResourceTextureView(m_shadowMaps2D[m_shadowMaps2D.size() - 1].get(), shadowMapViewDesc));
		}
	}



	void RenderLayer::UpdateSkybox(CSkybox& _skybox)
	{
		TextureViewDesc textureViewDesc{};
		textureViewDesc.dimension = TEXTURE_VIEW_DIMENSION::DIM_CUBE;
		textureViewDesc.format = DATA_FORMAT::R16G16B16A16_SFLOAT;
		textureViewDesc.type = TEXTURE_VIEW_TYPE::SHADER_READ_ONLY;
		textureViewDesc.baseArrayLayer = 0;
		textureViewDesc.arrayLayerCount = 6;
		
		
		if (_skybox.skyboxFilepathDirty)
		{
			ImageData* data{ TextureCompressor::LoadImage(_skybox.GetSkyboxFilepath(), false, true) };
			data->desc.usage |= TEXTURE_USAGE_FLAGS::READ_ONLY;
			m_skyboxTexture = m_device->CreateTexture(data->desc);
			m_gpuUploader->EnqueueTextureDataUpload(data->data, m_skyboxTexture.get(), RESOURCE_STATE::UNDEFINED);
			m_skyboxTextureView = m_device->CreateShaderResourceTextureView(m_skyboxTexture.get(), textureViewDesc);
			m_newGPUUploaderUpload = true;
			_skybox.skyboxFilepathDirty = false;
		}

		
		if (_skybox.irradianceFilepathDirty)
		{
			ImageData* data{ TextureCompressor::LoadImage(_skybox.GetIrradianceFilepath(), false, true) };
			data->desc.usage |= TEXTURE_USAGE_FLAGS::READ_ONLY;
			m_irradianceMap = m_device->CreateTexture(data->desc);
			m_gpuUploader->EnqueueTextureDataUpload(data->data, m_irradianceMap.get(), RESOURCE_STATE::UNDEFINED);
			m_irradianceMapView = m_device->CreateShaderResourceTextureView(m_irradianceMap.get(), textureViewDesc);
			m_newGPUUploaderUpload = true;
			_skybox.irradianceFilepathDirty = false;
		}

		
		if (_skybox.prefilterFilepathDirty)
		{
			ImageData* data{ TextureCompressor::LoadImage(_skybox.GetSkyboxFilepath(), false, true) };
			data->desc.usage |= TEXTURE_USAGE_FLAGS::READ_ONLY;
			m_prefilterMap = m_device->CreateTexture(data->desc);
			m_gpuUploader->EnqueueTextureDataUpload(data->data, m_prefilterMap.get(), RESOURCE_STATE::UNDEFINED);
			m_prefilterMapView = m_device->CreateShaderResourceTextureView(m_prefilterMap.get(), textureViewDesc);
			m_newGPUUploaderUpload = true;
			_skybox.prefilterFilepathDirty = false;
		}
		
	}



	void RenderLayer::UpdateCameraBuffer(const CCamera& _camera) const
	{
		//Check if the user has requested that the camera's aspect ratio be the window's aspect ratio
		constexpr float epsilon{ 0.1f }; //Buffer for floating-point-comparison-imprecision mitigation
		if (std::abs(_camera.camera->GetAspectRatio() - WIN_ASPECT_RATIO) < epsilon)
		{
			_camera.camera->SetAspectRatio(static_cast<float>(m_desc.window->GetSize().x) / m_desc.window->GetSize().y);
		}

		//Update m_camDataBufferMap with the camera data from this frame
		const CameraShaderData camShaderData{ _camera.camera->GetCurrentCameraShaderData(PROJECTION_METHOD::PERSPECTIVE) };
		memcpy(m_camDataBufferMaps[m_currentFrame], &camShaderData, sizeof(CameraShaderData));
		if (m_firstFrame)
		{
			for (std::size_t i{ 0 }; i < m_desc.framesInFlight; ++i)
			{
				m_graphicsCommandBuffers[0]->CopyBufferToBuffer(m_camDataBuffers[i].get(), m_camDataBuffersPreviousFrame[i].get(), 0, 0, sizeof(CameraShaderData));
			}
		}

		//Set aspect ratio back to WIN_ASPECT_RATIO for future lookups (can't keep it as the window's current aspect ratio in case it changes)
		_camera.camera->SetAspectRatio(WIN_ASPECT_RATIO);
	}



	void RenderLayer::UpdateLightDataBuffer()
	{
		bool bufferDirty{ false };
		m_cpuLightData.clear();
		for (auto&& [transform, light] : m_reg.get().View<CTransform, CLight>())
		{
			//Buffer is dirty if either transform.lightBufferDirty or light.light->dirty
			if (transform.lightBufferDirty || light.light->GetDirty())
			{
				bufferDirty = true;
			}

			if (light.light->GetShadowMapDirty())
			{
				InitShadowMapForLight(light);
				if (light.lightType == LIGHT_TYPE::POINT)
				{
					const std::size_t vecIdx = m_shadowMapsCube.size() - 1;
					light.light->SetShadowMapVectorIndex(vecIdx);
					light.light->SetShadowMapIndex(m_shadowMapCubeSRVs.back()->GetIndex());
				}
				else
				{
					const std::size_t vecIdx = m_shadowMaps2D.size() - 1;
					light.light->SetShadowMapVectorIndex(vecIdx);
					light.light->SetShadowMapIndex(m_shadowMap2DSRVs.back()->GetIndex());
				}
			}

			LightShaderData shaderData{};
			shaderData.colour = light.light->GetColour();
			shaderData.intensity = light.light->GetIntensity();
			shaderData.position = transform.GetPosition();
			shaderData.type = light.lightType;
			shaderData.shadowMapIndex = light.light->GetShadowMapIndex();

			//Calculate direction and view matrix from rotation
			const glm::quat orientation{ glm::quat(transform.GetRotation()) };
			shaderData.direction = glm::normalize(orientation * glm::vec3(0, 0, 1));
			const glm::vec3 forward{ glm::normalize(orientation * glm::vec3(0, 0, 1)) };
			const glm::vec3 up{ glm::normalize(orientation * glm::vec3(0, 1, 0)) };
			const glm::mat4 viewMat{ glm::lookAtLH(transform.GetPosition(), transform.GetPosition() + forward, up) };
			
			switch (light.lightType)
			{
			case LIGHT_TYPE::UNDEFINED:
			{
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::RENDER_LAYER, "UpdateLightDataBuffer() - light.lightType = LIGHT_TYPE::UNDEFINED\n");
				throw std::runtime_error("");
				break;
			}
			case LIGHT_TYPE::DIRECTIONAL:
			{
				const DirectionalLight* dirLight{ dynamic_cast<DirectionalLight*>(light.light.get()) };

				const glm::vec3& d{ dirLight->GetDimensions() };
				const glm::mat4 projMat{ glm::orthoLH(-d.x, d.x, -d.y, d.y, -d.z, d.z) };
				shaderData.viewProjMat = projMat * viewMat;
				
				break;
			}
			case LIGHT_TYPE::POINT:
			{
				const PointLight* pointLight{ dynamic_cast<PointLight*>(light.light.get()) };

				//Shadow mapping for point lights requires 6 draw calls to create a cubemap
				//View matrices are aligned to the world axes for point lights and are calculated at draw time
				//Projection matrix for all faces of all point light draw calls is constant - we use m_pointLightProjMatrix

				shaderData.constantAttenuation = pointLight->GetConstantAttenuation();
				shaderData.linearAttenuation = pointLight->GetLinearAttenuation();
				shaderData.quadraticAttenuation = pointLight->GetQuadraticAttenuation();
				
				break;
			}
			case LIGHT_TYPE::SPOT:
			{
				const SpotLight* spotLight{ dynamic_cast<SpotLight*>(light.light.get()) };

				const glm::mat4 projMat{ glm::perspectiveLH(glm::radians(spotLight->GetOuterAngle()), 1.0f, 0.01f, 100.0f) }; //todo: add range parameters for spot light shadow mapping
				shaderData.viewProjMat = projMat * viewMat;
				
				shaderData.constantAttenuation = spotLight->GetConstantAttenuation();
				shaderData.linearAttenuation = spotLight->GetLinearAttenuation();
				shaderData.quadraticAttenuation = spotLight->GetQuadraticAttenuation();

				shaderData.innerAngle = spotLight->GetInnerAngle();
				shaderData.outerAngle = spotLight->GetOuterAngle();
				
				break;
			}
			}

			transform.lightBufferDirty = false;
			light.light->SetDirty(false);
			m_cpuLightData.push_back(std::move(shaderData));
		}

		if (bufferDirty)
		{
			memcpy(m_lightDataBufferMap, m_cpuLightData.data(), sizeof(LightShaderData) * m_cpuLightData.size());
		}
	}



	void RenderLayer::UpdateModelMatricesBuffer()
	{
		m_modelMatrices.clear();
		m_modelMatricesEntitiesLookups[m_currentFrame].clear();

		for (auto&& [transform, model] : m_reg.get().View<CTransform, CModelRenderer>())
		{
			constexpr float epsilon{ 1e-3 };
			if (model.localSpaceHalfExtents.x < epsilon && model.localSpaceHalfExtents.y < epsilon && model.localSpaceHalfExtents.z < epsilon)
			{
				//Model boundary hasn't been set yet
				const CPUModel_SerialisedHeader header{ ModelLoader::GetNKModelHeader(model.modelPath) };
				model.localSpaceHalfExtents = header.halfExtents;
				model.localSpaceOrigin = glm::vec3(0);
			}

			constexpr float scaleBuffer{ 1.05f }; //Used as a scalar multiplier to the AABB's scale so that it's a bit larger than the model (eliminates z-fighting issues)
			const glm::mat4 aabbMatrix{ transform.GetModelMatrix() * glm::translate(glm::mat4(1.0f), model.localSpaceOrigin) * glm::scale(glm::mat4(1.0f), model.localSpaceHalfExtents * 2.0f * scaleBuffer) };
			m_modelMatrices.push_back(aabbMatrix);
			m_modelMatricesEntitiesLookups[m_currentFrame].push_back(m_reg.get().GetEntity(transform));
		}
		
		memcpy(m_modelMatricesBufferMaps[m_currentFrame], m_modelMatrices.data(), m_modelMatrices.size() * sizeof(glm::mat4));
	}



	glm::mat4 RenderLayer::GetPointLightViewMatrix(const glm::vec3& _lightPos, const std::size_t _faceIndex)
	{
		//Standard cubemap face order: +X, -X, +Y, -Y, +Z, -Z
		switch (_faceIndex)
		{
		case 0: return glm::lookAtLH(_lightPos, _lightPos + glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, 1.0f,  0.0f)); //+X
		case 1: return glm::lookAtLH(_lightPos, _lightPos + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, 1.0f,  0.0f)); //-X
		case 2: return glm::lookAtLH(_lightPos, _lightPos + glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)); //+Y
		case 3: return glm::lookAtLH(_lightPos, _lightPos + glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)); //-Y
		case 4: return glm::lookAtLH(_lightPos, _lightPos + glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, 1.0f,  0.0f)); //+Z
		case 5: return glm::lookAtLH(_lightPos, _lightPos + glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, 1.0f,  0.0f)); //-Z
		default:	{ throw std::invalid_argument("RenderLayer::GetPointLightViewMatrix() - _faceIndex (" + std::to_string(_faceIndex) + ")" + " is not in the allowed range of 0 to 5"); }
		}
	}

}