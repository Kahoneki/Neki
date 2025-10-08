#include <cstring>
#include <Core/RAIIContext.h>
#include <Core/Debug/ILogger.h>
#include <Core/Memory/Allocation.h>
#include <Core/Memory/TrackingAllocator.h>
#include <Core/Utils/FormatUtils.h>
#include <Graphics/Camera/PlayerCamera.h>
#include <Managers/InputManager.h>
#include <Managers/TimeManager.h>
#include <RHI-Vulkan/VulkanDevice.h>
#include <RHI/IBuffer.h>
#include <RHI/IBufferView.h>
#include <RHI/IPipeline.h>
#include <RHI/ISampler.h>
#include <RHI/ISemaphore.h>
#include <RHI/IShader.h>
#include <RHI/ISurface.h>
#include <RHI/ISwapchain.h>
#include <RHI/ITexture.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>


int main()
{
	NK::LoggerConfig loggerConfig{ NK::LOGGER_TYPE::CONSOLE, true };
	loggerConfig.SetLayerChannelBitfield(NK::LOGGER_LAYER::VULKAN_GENERAL, NK::LOGGER_CHANNEL::WARNING | NK::LOGGER_CHANNEL::ERROR);
	loggerConfig.SetLayerChannelBitfield(NK::LOGGER_LAYER::VULKAN_VALIDATION, NK::LOGGER_CHANNEL::WARNING | NK::LOGGER_CHANNEL::ERROR);
	loggerConfig.SetLayerChannelBitfield(NK::LOGGER_LAYER::TRACKING_ALLOCATOR, NK::LOGGER_CHANNEL::WARNING | NK::LOGGER_CHANNEL::ERROR);
	NK::RAIIContext context{ loggerConfig, NK::ALLOCATOR_TYPE::TRACKING_VERBOSE };
	NK::ILogger* logger{ NK::Context::GetLogger() };
	NK::IAllocator* allocator{ NK::Context::GetAllocator() };

	logger->Unindent();

	//Device
	const NK::UniquePtr<NK::IDevice> device{ NK_NEW(NK::VulkanDevice, *logger, *allocator) };

	//Graphics Command Pool
	NK::CommandPoolDesc graphicsCommandPoolDesc{};
	graphicsCommandPoolDesc.type = NK::COMMAND_TYPE::GRAPHICS;
	const NK::UniquePtr<NK::ICommandPool> graphicsCommandPool{ device->CreateCommandPool(graphicsCommandPoolDesc) };

	//Primary Level Command Buffer Description
	NK::CommandBufferDesc primaryLevelCommandBufferDesc{};
	primaryLevelCommandBufferDesc.level = NK::COMMAND_BUFFER_LEVEL::PRIMARY;

	//Graphics Queue
	NK::QueueDesc graphicsQueueDesc{};
	graphicsQueueDesc.type = NK::COMMAND_TYPE::GRAPHICS;
	const NK::UniquePtr<NK::IQueue> graphicsQueue(device->CreateQueue(graphicsQueueDesc));

	//Transfer Queue
	NK::QueueDesc transferQueueDesc{};
	transferQueueDesc.type = NK::COMMAND_TYPE::TRANSFER;
	const NK::UniquePtr<NK::IQueue> transferQueue{ device->CreateQueue(transferQueueDesc) };

	//GPU Uploader
	NK::GPUUploaderDesc gpuUploaderDesc{};
	gpuUploaderDesc.stagingBufferSize = 1024 * 512 * 512; //512MiB
	gpuUploaderDesc.transferQueue = transferQueue.get();
	const NK::UniquePtr<NK::GPUUploader> gpuUploader{ device->CreateGPUUploader(gpuUploaderDesc) };

	//Window and Surface
	NK::WindowDesc windowDesc{};
	windowDesc.name = "Model Sample";
	constexpr glm::ivec2 SCREEN_DIMENSIONS{ 1920, 1080 };
	windowDesc.size = glm::ivec2(SCREEN_DIMENSIONS.x, SCREEN_DIMENSIONS.y);
	const NK::UniquePtr<NK::Window> window{ device->CreateWindow(windowDesc) };
	const NK::UniquePtr<NK::ISurface> surface{ device->CreateSurface(window.get()) };

	//Swapchain
	NK::SwapchainDesc swapchainDesc{};
	swapchainDesc.surface = surface.get();
	swapchainDesc.numBuffers = 3;
	swapchainDesc.presentQueue = graphicsQueue.get();
	const NK::UniquePtr<NK::ISwapchain> swapchain{ device->CreateSwapchain(swapchainDesc) };

	//Camera
	NK::PlayerCamera camera{ NK::PlayerCamera(glm::vec3(0, 0, 3), 0, 0, 0.01f, 100.0f, 90.0f, static_cast<float>(SCREEN_DIMENSIONS.x) / SCREEN_DIMENSIONS.y, 30.0f, 0.05f) };

	//Camera Data Buffer
	NK::CameraShaderData initialCamShaderData{ camera.GetCameraShaderData(NK::PROJECTION_METHOD::PERSPECTIVE) };
	NK::BufferDesc camDataBufferDesc{};
	camDataBufferDesc.size = sizeof(initialCamShaderData);
	camDataBufferDesc.type = NK::MEMORY_TYPE::HOST; //Todo: look into device-local host-accessible memory type?
	camDataBufferDesc.usage = NK::BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | NK::BUFFER_USAGE_FLAGS::UNIFORM_BUFFER_BIT;
	const NK::UniquePtr<NK::IBuffer> camDataBuffer{ device->CreateBuffer(camDataBufferDesc) };
	void* camDataBufferMap{ camDataBuffer->Map() };
	memcpy(camDataBufferMap, &initialCamShaderData, sizeof(initialCamShaderData));

	//Camera Data Buffer View
	NK::BufferViewDesc camDataBufferViewDesc{};
	camDataBufferViewDesc.size = sizeof(initialCamShaderData);
	camDataBufferViewDesc.offset = 0;
	camDataBufferViewDesc.type = NK::BUFFER_VIEW_TYPE::UNIFORM;
	const NK::UniquePtr<NK::IBufferView> camDataBufferView{ device->CreateBufferView(camDataBuffer.get(), camDataBufferViewDesc) };
	NK::ResourceIndex camDataBufferIndex{ camDataBufferView->GetIndex() };

	//Vertex Shader
	NK::ShaderDesc vertShaderDesc{};
	vertShaderDesc.type = NK::SHADER_TYPE::VERTEX;
	vertShaderDesc.filepath = "Samples/Shaders/Model/Model_vs";
	const NK::UniquePtr<NK::IShader> vertShader{ device->CreateShader(vertShaderDesc) };


	//Fragment Shaders
	NK::ShaderDesc blinnPhongFragShaderDesc{};
	blinnPhongFragShaderDesc.type = NK::SHADER_TYPE::FRAGMENT;
	blinnPhongFragShaderDesc.filepath = "Samples/Shaders/Model/ModelBlinnPhong_fs";
	const NK::UniquePtr<NK::IShader> blinnPhongFragShader{ device->CreateShader(blinnPhongFragShaderDesc) };

	NK::ShaderDesc pbrFragShaderDesc{};
	pbrFragShaderDesc.type = NK::SHADER_TYPE::FRAGMENT;
	pbrFragShaderDesc.filepath = "Samples/Shaders/Model/ModelPBR_fs";
	const NK::UniquePtr<NK::IShader> pbrFragShader{ device->CreateShader(pbrFragShaderDesc) };


	//Root Signature
	NK::RootSignatureDesc rootSigDesc{};
	rootSigDesc.num32BitPushConstantValues = 16 + 1 + 1 + 1; //model matrix + cam data buffer index + material buffer index + sampler index
	const NK::UniquePtr<NK::IRootSignature> rootSig{ device->CreateRootSignature(rootSigDesc) };


	//Model
	const NK::CPUModel* const modelData{ NK::ModelLoader::LoadModel("Samples/Resource Files/DamagedHelmet/DamagedHelmet.gltf", false, true) };
	const NK::UniquePtr<NK::GPUModel> model{ gpuUploader->EnqueueModelDataUpload(modelData) };

	//Flush gpu uploader uploads
	gpuUploader->Flush(true);
	gpuUploader->Reset();

	//Model model matrix (like... the... model matrix of the... model)
	glm::mat4 modelModelMatrix{ glm::mat4(1.0f) };
	modelModelMatrix = glm::rotate(modelModelMatrix, glm::radians(30.0f), glm::vec3(0, -1, 0));
	modelModelMatrix = glm::rotate(modelModelMatrix, glm::radians(70.0f), glm::vec3(1, 0, 0));

	//Sampler
	NK::SamplerDesc samplerDesc{};
	samplerDesc.minFilter = NK::FILTER_MODE::LINEAR;
	samplerDesc.magFilter = NK::FILTER_MODE::LINEAR;
	const NK::UniquePtr<NK::ISampler> sampler{ device->CreateSampler(samplerDesc) };
	NK::SamplerIndex samplerIndex{ sampler->GetIndex() };


	//Graphics Pipeline

	NK::VertexInputDesc vertexInputDesc{ NK::ModelLoader::GetModelVertexInputDescription() };

	NK::InputAssemblyDesc inputAssemblyDesc{};
	inputAssemblyDesc.topology = NK::INPUT_TOPOLOGY::TRIANGLE_LIST;

	NK::RasteriserDesc rasteriserDesc{};
	rasteriserDesc.cullMode = NK::CULL_MODE::BACK;
	rasteriserDesc.frontFace = NK::WINDING_DIRECTION::CLOCKWISE;
	rasteriserDesc.depthBiasEnable = false;

	NK::DepthStencilDesc depthStencilDesc{};
	depthStencilDesc.depthTestEnable = true;
	depthStencilDesc.depthWriteEnable = true;
	depthStencilDesc.depthCompareOp = NK::COMPARE_OP::LESS;
	depthStencilDesc.stencilTestEnable = false;

	NK::MultisamplingDesc multisamplingDesc{};
	multisamplingDesc.sampleCount = NK::SAMPLE_COUNT::BIT_1;
	multisamplingDesc.sampleMask = UINT32_MAX;
	multisamplingDesc.alphaToCoverageEnable = false;

	std::vector<NK::ColourBlendAttachmentDesc> colourBlendAttachments(1);
	colourBlendAttachments[0].colourWriteMask = NK::COLOUR_ASPECT_FLAGS::R_BIT | NK::COLOUR_ASPECT_FLAGS::G_BIT | NK::COLOUR_ASPECT_FLAGS::B_BIT | NK::COLOUR_ASPECT_FLAGS::A_BIT;
	colourBlendAttachments[0].blendEnable = false;
	NK::ColourBlendDesc colourBlendDesc{};
	colourBlendDesc.logicOpEnable = false;
	colourBlendDesc.attachments = colourBlendAttachments;

	NK::PipelineDesc graphicsPipelineDesc{};
	graphicsPipelineDesc.type = NK::PIPELINE_TYPE::GRAPHICS;
	graphicsPipelineDesc.vertexShader = vertShader.get();
	graphicsPipelineDesc.fragmentShader = blinnPhongFragShader.get();
	graphicsPipelineDesc.rootSignature = rootSig.get();
	graphicsPipelineDesc.vertexInputDesc = vertexInputDesc;
	graphicsPipelineDesc.inputAssemblyDesc = inputAssemblyDesc;
	graphicsPipelineDesc.rasteriserDesc = rasteriserDesc;
	graphicsPipelineDesc.depthStencilDesc = depthStencilDesc;
	graphicsPipelineDesc.multisamplingDesc = multisamplingDesc;
	graphicsPipelineDesc.colourBlendDesc = colourBlendDesc;
	graphicsPipelineDesc.colourAttachmentFormats = { NK::DATA_FORMAT::R8G8B8A8_SRGB };
	graphicsPipelineDesc.depthStencilAttachmentFormat = NK::DATA_FORMAT::D32_SFLOAT;

	const NK::UniquePtr<NK::IPipeline> blinnPhongPipeline{ device->CreatePipeline(graphicsPipelineDesc) };

	graphicsPipelineDesc.fragmentShader = pbrFragShader.get();
	const NK::UniquePtr<NK::IPipeline> pbrPipeline{ device->CreatePipeline(graphicsPipelineDesc) };


	//Depth Buffer
	NK::TextureDesc depthBufferDesc{};
	depthBufferDesc.dimension = NK::TEXTURE_DIMENSION::DIM_2;
	depthBufferDesc.format = NK::DATA_FORMAT::D32_SFLOAT;
	depthBufferDesc.size = glm::ivec3(SCREEN_DIMENSIONS.x, SCREEN_DIMENSIONS.y, 1);
	depthBufferDesc.usage = NK::TEXTURE_USAGE_FLAGS::DEPTH_STENCIL_ATTACHMENT;
	depthBufferDesc.arrayTexture = false;
	const NK::UniquePtr<NK::ITexture> depthBuffer{ device->CreateTexture(depthBufferDesc) };

	//Depth Buffer View
	NK::TextureViewDesc depthBufferViewDesc{};
	depthBufferViewDesc.dimension = NK::TEXTURE_DIMENSION::DIM_2;
	depthBufferViewDesc.format = NK::DATA_FORMAT::D32_SFLOAT;
	depthBufferViewDesc.type = NK::TEXTURE_VIEW_TYPE::DEPTH;
	const NK::UniquePtr<NK::ITextureView> depthBufferView{ device->CreateDepthStencilTextureView(depthBuffer.get(), depthBufferViewDesc) };


	//Number of frames the CPU can get ahead of the GPU
	constexpr std::uint32_t MAX_FRAMES_IN_FLIGHT{ 10 };

	//Graphics Command Buffers
	std::vector<NK::UniquePtr<NK::ICommandBuffer>> commandBuffers(MAX_FRAMES_IN_FLIGHT);
	for (std::size_t i{ 0 }; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		commandBuffers[i] = graphicsCommandPool->AllocateCommandBuffer(primaryLevelCommandBufferDesc);
	}

	//Semaphores
	std::vector<NK::UniquePtr<NK::ISemaphore>> imageAvailableSemaphores(MAX_FRAMES_IN_FLIGHT);
	for (std::size_t i{ 0 }; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		imageAvailableSemaphores[i] = device->CreateSemaphore();
	}

	std::vector<NK::UniquePtr<NK::ISemaphore>> renderFinishedSemaphores(swapchain->GetNumImages());
	for (std::size_t i{ 0 }; i < swapchain->GetNumImages(); ++i)
	{
		renderFinishedSemaphores[i] = device->CreateSemaphore();
	}


	//Fences
	NK::FenceDesc inFlightFenceDesc{};
	inFlightFenceDesc.initiallySignaled = true;
	std::vector<NK::UniquePtr<NK::IFence>> inFlightFences(MAX_FRAMES_IN_FLIGHT);
	for (std::size_t i{ 0 }; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		inFlightFences[i] = device->CreateFence(inFlightFenceDesc);
	}

	//Tracks the current frame in range [0, MAX_FRAMES_IN_FLIGHT-1]
	std::uint32_t currentFrame{ 0 };

	while (!window->ShouldClose())
	{
		//Update managers
		glfwPollEvents();
		NK::TimeManager::Update();
		NK::InputManager::Update(window.get());


		//Update player camera
		camera.Update(window.get());
		NK::CameraShaderData camShaderData{ camera.GetCameraShaderData(NK::PROJECTION_METHOD::PERSPECTIVE) };
		memcpy(camDataBufferMap, &camShaderData, sizeof(camShaderData));


		//Begin rendering
		inFlightFences[currentFrame]->Wait();
		inFlightFences[currentFrame]->Reset();

		commandBuffers[currentFrame]->Begin();
		std::uint32_t imageIndex{ swapchain->AcquireNextImageIndex(imageAvailableSemaphores[currentFrame].get(), nullptr) };
		commandBuffers[currentFrame]->TransitionBarrier(swapchain->GetImage(imageIndex), NK::RESOURCE_STATE::UNDEFINED, NK::RESOURCE_STATE::RENDER_TARGET);

		commandBuffers[currentFrame]->BeginRendering(1, swapchain->GetImageView(imageIndex), depthBufferView.get(), nullptr);
		commandBuffers[currentFrame]->BindRootSignature(rootSig.get(), NK::PIPELINE_BIND_POINT::GRAPHICS);

		commandBuffers[currentFrame]->SetViewport({ 0, 0 }, { SCREEN_DIMENSIONS.x, SCREEN_DIMENSIONS.y });
		commandBuffers[currentFrame]->SetScissor({ 0, 0 }, { SCREEN_DIMENSIONS.x, SCREEN_DIMENSIONS.y });


		//Drawing
		struct PushConstantData
		{
			glm::mat4 modelMat;
			NK::ResourceIndex camDataBufferIndex;
			NK::ResourceIndex materialBufferIndex;
			NK::SamplerIndex samplerIndex;
		};
		std::size_t vertexBufferStride{ sizeof(NK::ModelVertex) };

		for (std::size_t i{ 0 }; i < model->meshes.size(); ++i)
		{
			NK::IPipeline* pipeline{ model->materials[0]->lightingModel == NK::LIGHTING_MODEL::BLINN_PHONG ? blinnPhongPipeline.get() : pbrPipeline.get() };
			commandBuffers[currentFrame]->BindPipeline(pipeline, NK::PIPELINE_BIND_POINT::GRAPHICS);

			commandBuffers[currentFrame]->BindVertexBuffers(0, 1, model->meshes[i]->vertexBuffer.get(), &vertexBufferStride);
			commandBuffers[currentFrame]->BindIndexBuffer(model->meshes[i]->indexBuffer.get(), NK::DATA_FORMAT::R32_UINT);

			PushConstantData pushConstantData{};
			float speed{ 50.0f };
			modelModelMatrix = glm::rotate(modelModelMatrix, glm::radians(speed * static_cast<float>(NK::TimeManager::GetDeltaTime())), glm::vec3(0, 0, 1));
			pushConstantData.modelMat = modelModelMatrix;
			pushConstantData.camDataBufferIndex = camDataBufferIndex;
			pushConstantData.materialBufferIndex = model->materials[model->meshes[i]->materialIndex]->bufferIndex;
			pushConstantData.samplerIndex = samplerIndex;
			commandBuffers[currentFrame]->PushConstants(rootSig.get(), &pushConstantData);

			commandBuffers[currentFrame]->DrawIndexed(model->meshes[i]->indexCount, 1, 0, 0);
		}


		//End rendering
		commandBuffers[currentFrame]->EndRendering();
		commandBuffers[currentFrame]->TransitionBarrier(swapchain->GetImage(imageIndex), NK::RESOURCE_STATE::RENDER_TARGET, NK::RESOURCE_STATE::PRESENT);
		commandBuffers[currentFrame]->End();


		//Submit
		graphicsQueue->Submit(commandBuffers[currentFrame].get(), imageAvailableSemaphores[currentFrame].get(), renderFinishedSemaphores[imageIndex].get(), inFlightFences[currentFrame].get());
		swapchain->Present(renderFinishedSemaphores[imageIndex].get(), imageIndex);

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	}
	camDataBuffer->Unmap();
	graphicsQueue->WaitIdle();
}