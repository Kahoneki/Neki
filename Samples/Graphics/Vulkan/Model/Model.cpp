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

	//----SETTINGS----//
	constexpr glm::ivec2 SCREEN_DIMENSIONS{ 1920, 1080 };
	constexpr bool enableMSAA{ false };
	constexpr bool enableSSAA{ true };
	constexpr NK::SAMPLE_COUNT msaaSampleCount{ NK::SAMPLE_COUNT::BIT_8 };
	constexpr std::uint32_t ssaaMultiplier{ 4u }; //16x SSAA
	//----------------//

	
	constexpr glm::ivec2 SUPERSAMPLE_RESOLUTION{ SCREEN_DIMENSIONS * glm::ivec2(ssaaMultiplier, ssaaMultiplier) };
	
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
	windowDesc.size = SCREEN_DIMENSIONS;
	const NK::UniquePtr<NK::Window> window{ device->CreateWindow(windowDesc) };
	const NK::UniquePtr<NK::ISurface> surface{ device->CreateSurface(window.get()) };

	//Swapchain
	NK::SwapchainDesc swapchainDesc{};
	swapchainDesc.surface = surface.get();
	swapchainDesc.numBuffers = 3;
	swapchainDesc.presentQueue = graphicsQueue.get();
	const NK::UniquePtr<NK::ISwapchain> swapchain{ device->CreateSwapchain(swapchainDesc) };

	//Camera
	NK::PlayerCamera camera{ NK::PlayerCamera(glm::vec3(0, 0, 3), -90.0f, 0, 0.1f, 5000.0f, 90.0f, static_cast<float>(SCREEN_DIMENSIONS.x) / SCREEN_DIMENSIONS.y, 300.0f, 0.05f) };

	//Camera Data Buffer
	NK::CameraShaderData initialCamShaderData{ camera.GetCurrentCameraShaderData(NK::PROJECTION_METHOD::PERSPECTIVE) };
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

	//Vertex Shaders
	NK::ShaderDesc vertShaderDesc{};
	vertShaderDesc.type = NK::SHADER_TYPE::VERTEX;
	vertShaderDesc.filepath = "Samples/Shaders/Model/Model_vs";
	const NK::UniquePtr<NK::IShader> vertShader{ device->CreateShader(vertShaderDesc) };

	vertShaderDesc.filepath = "Samples/Shaders/Model/Skybox_vs";
	const NK::UniquePtr<NK::IShader> skyboxVertShader{ device->CreateShader(vertShaderDesc) };


	//Fragment Shaders
	NK::ShaderDesc fragShaderDesc{};
	fragShaderDesc.type = NK::SHADER_TYPE::FRAGMENT;
	fragShaderDesc.filepath = "Samples/Shaders/Model/ModelBlinnPhong_fs";
	const NK::UniquePtr<NK::IShader> blinnPhongFragShader{ device->CreateShader(fragShaderDesc) };

	fragShaderDesc.filepath = "Samples/Shaders/Model/ModelPBR_fs";
	const NK::UniquePtr<NK::IShader> pbrFragShader{ device->CreateShader(fragShaderDesc) };

	fragShaderDesc.filepath = "Samples/Shaders/Model/Skybox_fs";
	const NK::UniquePtr<NK::IShader> skyboxFragShader{ device->CreateShader(fragShaderDesc) };


	//Root Signature
	NK::RootSignatureDesc rootSigDesc{};
	rootSigDesc.num32BitPushConstantValues = 16 + 1 + 1 + 1 + 1 + 1; //model matrix + cam data buffer index + skybox cubemap index + material buffer index + sampler index
	const NK::UniquePtr<NK::IRootSignature> rootSig{ device->CreateRootSignature(rootSigDesc) };
	
	//Skybox Texture
	void* skyboxImageData[6]
	{
		NK::ImageLoader::LoadImage("Samples/Resource Files/skybox/right.jpg", false, true)->data,
		NK::ImageLoader::LoadImage("Samples/Resource Files/skybox/left.jpg", false, true)->data,
		NK::ImageLoader::LoadImage("Samples/Resource Files/skybox/top.jpg", false, true)->data,
		NK::ImageLoader::LoadImage("Samples/Resource Files/skybox/bottom.jpg", false, true)->data,
		NK::ImageLoader::LoadImage("Samples/Resource Files/skybox/front.jpg", false, true)->data,
		NK::ImageLoader::LoadImage("Samples/Resource Files/skybox/back.jpg", false, true)->data
	};
	NK::TextureDesc skyboxTextureDesc{ NK::ImageLoader::LoadImage("Samples/Resource Files/skybox/right.jpg", true, true)->desc }; //cached, very inexpensive load
	skyboxTextureDesc.usage |= NK::TEXTURE_USAGE_FLAGS::READ_ONLY;
	skyboxTextureDesc.arrayTexture = true;
	skyboxTextureDesc.size.z = 6;
	skyboxTextureDesc.cubemap = true;
	const NK::UniquePtr<NK::ITexture> skyboxTexture{ device->CreateTexture(skyboxTextureDesc) };
	gpuUploader->EnqueueArrayTextureDataUpload(skyboxImageData, skyboxTexture.get(), NK::RESOURCE_STATE::UNDEFINED);
	
	//Skybox Texture View
	NK::TextureViewDesc skyboxTextureViewDesc{};
	skyboxTextureViewDesc.dimension = NK::TEXTURE_VIEW_DIMENSION::DIM_CUBE;
	skyboxTextureViewDesc.format = NK::DATA_FORMAT::R8G8B8A8_SRGB;
	skyboxTextureViewDesc.type = NK::TEXTURE_VIEW_TYPE::SHADER_READ_ONLY;
	skyboxTextureViewDesc.baseArrayLayer = 0;
	skyboxTextureViewDesc.arrayLayerCount = 6;
	const NK::UniquePtr<NK::ITextureView> skyboxTextureView{ device->CreateShaderResourceTextureView(skyboxTexture.get(), skyboxTextureViewDesc) };
	NK::ResourceIndex skyboxTextureResourceIndex{ skyboxTextureView->GetIndex() };

	//Model
	const NK::CPUModel* const modelData{ NK::ModelLoader::LoadModel("Samples/Resource Files/Sponza/Sponza.gltf", true, true) };
	const NK::UniquePtr<NK::GPUModel> model{ gpuUploader->EnqueueModelDataUpload(modelData) };


	//Skybox cube
	//Vertex Buffer
	constexpr glm::vec3 vertices[8] =
	{
		glm::vec3(-1.0f, -1.0f,  1.0f), //0: Front-Left-Bottom
		glm::vec3( 1.0f, -1.0f,  1.0f), //1: Front-Right-Bottom
		glm::vec3( 1.0f,  1.0f,  1.0f), //2: Front-Right-Top
		glm::vec3(-1.0f,  1.0f,  1.0f), //3: Front-Left-Top
		glm::vec3(-1.0f, -1.0f, -1.0f), //4: Back-Left-Bottom
		glm::vec3( 1.0f, -1.0f, -1.0f), //5: Back-Right-Bottom
		glm::vec3( 1.0f,  1.0f, -1.0f), //6: Back-Right-Top
		glm::vec3(-1.0f,  1.0f, -1.0f)  //7: Back-Left-Top
	};

	NK::BufferDesc skyboxVertBufferDesc{};
	skyboxVertBufferDesc.size = sizeof(glm::vec3) * 8;
	skyboxVertBufferDesc.type = NK::MEMORY_TYPE::DEVICE;
	skyboxVertBufferDesc.usage = NK::BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | NK::BUFFER_USAGE_FLAGS::VERTEX_BUFFER_BIT;
	const NK::UniquePtr<NK::IBuffer> skyboxVertBuffer{ device->CreateBuffer(skyboxVertBufferDesc) };

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

	NK::BufferDesc indexBufferDesc{};
	indexBufferDesc.size = sizeof(std::uint32_t) * 36;
	indexBufferDesc.type = NK::MEMORY_TYPE::DEVICE;
	indexBufferDesc.usage = NK::BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | NK::BUFFER_USAGE_FLAGS::INDEX_BUFFER_BIT;
	const NK::UniquePtr<NK::IBuffer> skyboxIndexBuffer{ device->CreateBuffer(indexBufferDesc) };
	
	//Upload vertex and index buffers
	gpuUploader->EnqueueBufferDataUpload(vertices, skyboxVertBuffer.get(), NK::RESOURCE_STATE::UNDEFINED);
	gpuUploader->EnqueueBufferDataUpload(indices, skyboxIndexBuffer.get(), NK::RESOURCE_STATE::UNDEFINED);
	
	
	//Flush gpu uploader uploads
	gpuUploader->Flush(true);
	gpuUploader->Reset();

	
	//Model model matrix (like... the... model matrix of the... model)
	glm::mat4 modelModelMatrix{ glm::mat4(1.0f) };

	//Kaju
//	modelModelMatrix = glm::rotate(modelModelMatrix, glm::radians(180.0f), glm::vec3(0, 1, 0));
//	modelModelMatrix = glm::rotate(modelModelMatrix, glm::radians(-90.0f), glm::vec3(1, 0, 0));
//	modelModelMatrix = glm::scale(modelModelMatrix, glm::vec3(0.1f));

	//Damaged Helmet
//	modelModelMatrix = glm::rotate(modelModelMatrix, glm::radians(180.0f), glm::vec3(0, 0, 1));
//	modelModelMatrix = glm::rotate(modelModelMatrix, glm::radians(30.0f), glm::vec3(0, -1, 0));
//	modelModelMatrix = glm::rotate(modelModelMatrix, glm::radians(70.0f), glm::vec3(1, 0, 0));
	
	
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
	multisamplingDesc.sampleCount = enableMSAA ? msaaSampleCount : NK::SAMPLE_COUNT::BIT_1;
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

	//Skybox pipeline
	std::vector<NK::VertexAttributeDesc> vertexAttributes;
	NK::VertexAttributeDesc posAttribute{};
	posAttribute.attribute = NK::SHADER_ATTRIBUTE::POSITION;
	posAttribute.binding = 0;
	posAttribute.format = NK::DATA_FORMAT::R32G32B32_SFLOAT;
	posAttribute.offset = 0;
	vertexAttributes.push_back(posAttribute);
	std::vector<NK::VertexBufferBindingDesc> bufferBindings;
	NK::VertexBufferBindingDesc bufferBinding{};
	bufferBinding.binding = 0;
	bufferBinding.inputRate = NK::VERTEX_INPUT_RATE::VERTEX;
	bufferBinding.stride = sizeof(glm::vec3);
	bufferBindings.push_back(bufferBinding);
	NK::VertexInputDesc skyboxVertexInputDesc{};
	skyboxVertexInputDesc.attributeDescriptions = vertexAttributes;
	skyboxVertexInputDesc.bufferBindingDescriptions = bufferBindings;
	graphicsPipelineDesc.vertexInputDesc = skyboxVertexInputDesc;
	graphicsPipelineDesc.rasteriserDesc.cullMode = NK::CULL_MODE::FRONT;
	graphicsPipelineDesc.depthStencilDesc.depthCompareOp = NK::COMPARE_OP::LESS_OR_EQUAL;
	graphicsPipelineDesc.vertexShader = skyboxVertShader.get();
	graphicsPipelineDesc.fragmentShader = skyboxFragShader.get();
	const NK::UniquePtr<NK::IPipeline> skyboxPipeline{ device->CreatePipeline(graphicsPipelineDesc) };
	
	

	//Render Target
	NK::TextureDesc renderTargetDesc{};
	renderTargetDesc.dimension = NK::TEXTURE_DIMENSION::DIM_2;
	renderTargetDesc.format = NK::DATA_FORMAT::R8G8B8A8_SRGB;
	renderTargetDesc.size = enableSSAA ? glm::ivec3(SUPERSAMPLE_RESOLUTION, 1) : glm::ivec3(SCREEN_DIMENSIONS, 1);
	renderTargetDesc.usage = NK::TEXTURE_USAGE_FLAGS::COLOUR_ATTACHMENT | NK::TEXTURE_USAGE_FLAGS::TRANSFER_SRC_BIT; //Needs TRANSFER_SRC_BIT for supersampling algorithm
	renderTargetDesc.arrayTexture = false;
	renderTargetDesc.sampleCount = enableMSAA ? msaaSampleCount : NK::SAMPLE_COUNT::BIT_1;
	const NK::UniquePtr<NK::ITexture> renderTarget{ device->CreateTexture(renderTargetDesc) };

	//Render Target View
	NK::TextureViewDesc renderTargetViewDesc{};
	renderTargetViewDesc.dimension = NK::TEXTURE_VIEW_DIMENSION::DIM_2;
	renderTargetViewDesc.format = NK::DATA_FORMAT::R8G8B8A8_SRGB;
	renderTargetViewDesc.type = NK::TEXTURE_VIEW_TYPE::RENDER_TARGET;
	const NK::UniquePtr<NK::ITextureView> renderTargetView{ device->CreateRenderTargetTextureView(renderTarget.get(), renderTargetViewDesc) };

	//Depth Buffer
	NK::TextureDesc depthBufferDesc{};
	depthBufferDesc.dimension = NK::TEXTURE_DIMENSION::DIM_2;
	depthBufferDesc.format = NK::DATA_FORMAT::D32_SFLOAT;
	depthBufferDesc.size = enableSSAA ? glm::ivec3(SUPERSAMPLE_RESOLUTION, 1) : glm::ivec3(SCREEN_DIMENSIONS, 1);
	depthBufferDesc.usage = NK::TEXTURE_USAGE_FLAGS::DEPTH_STENCIL_ATTACHMENT;
	depthBufferDesc.arrayTexture = false;
	depthBufferDesc.sampleCount = enableMSAA ? msaaSampleCount : NK::SAMPLE_COUNT::BIT_1;
	const NK::UniquePtr<NK::ITexture> depthBuffer{ device->CreateTexture(depthBufferDesc) };

	//Supersample Depth Buffer View
	NK::TextureViewDesc depthBufferViewDesc{};
	depthBufferViewDesc.dimension = NK::TEXTURE_VIEW_DIMENSION::DIM_2;
	depthBufferViewDesc.format = NK::DATA_FORMAT::D32_SFLOAT;
	depthBufferViewDesc.type = NK::TEXTURE_VIEW_TYPE::DEPTH;
	const NK::UniquePtr<NK::ITextureView> depthBufferView{ device->CreateDepthStencilTextureView(depthBuffer.get(), depthBufferViewDesc) };


	//Number of frames the CPU can get ahead of the GPU
	constexpr std::uint32_t MAX_FRAMES_IN_FLIGHT{ 3 };

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
		NK::CameraShaderData camShaderData{ camera.GetCurrentCameraShaderData(NK::PROJECTION_METHOD::PERSPECTIVE) };
		memcpy(camDataBufferMap, &camShaderData, sizeof(camShaderData));


		//Begin rendering
		inFlightFences[currentFrame]->Wait();
		inFlightFences[currentFrame]->Reset();

		commandBuffers[currentFrame]->Begin();
		std::uint32_t imageIndex{ swapchain->AcquireNextImageIndex(imageAvailableSemaphores[currentFrame].get(), nullptr) };

		if constexpr (enableMSAA || enableSSAA)
		{
			commandBuffers[currentFrame]->TransitionBarrier(renderTarget.get(), NK::RESOURCE_STATE::UNDEFINED, NK::RESOURCE_STATE::RENDER_TARGET);
		}
		commandBuffers[currentFrame]->TransitionBarrier(swapchain->GetImage(imageIndex), NK::RESOURCE_STATE::UNDEFINED, NK::RESOURCE_STATE::RENDER_TARGET);
		commandBuffers[currentFrame]->TransitionBarrier(depthBuffer.get(), NK::RESOURCE_STATE::UNDEFINED, NK::RESOURCE_STATE::DEPTH_WRITE);
		
		commandBuffers[currentFrame]->BeginRendering(1, enableMSAA ? renderTargetView.get() : nullptr, enableSSAA ? renderTargetView.get() : swapchain->GetImageView(imageIndex), depthBufferView.get(), nullptr);
		commandBuffers[currentFrame]->BindRootSignature(rootSig.get(), NK::PIPELINE_BIND_POINT::GRAPHICS);
		
		commandBuffers[currentFrame]->SetViewport({ 0, 0 }, { enableSSAA ? SUPERSAMPLE_RESOLUTION : SCREEN_DIMENSIONS });
		commandBuffers[currentFrame]->SetScissor({ 0, 0 }, { enableSSAA ? SUPERSAMPLE_RESOLUTION : SCREEN_DIMENSIONS });


		//Drawing
		struct PushConstantData
		{
			glm::mat4 modelMat;
			NK::ResourceIndex camDataBufferIndex;
			NK::ResourceIndex skyboxCubemapIndex;
			NK::ResourceIndex materialBufferIndex;
			NK::SamplerIndex samplerIndex;
		};
		std::size_t skyboxVertexBufferStride{ sizeof(glm::vec3) };
		std::size_t modelVertexBufferStride{ sizeof(NK::ModelVertex) };

		for (std::size_t i{ 0 }; i < model->meshes.size(); ++i)
		{
			//Push constants (unchanging between skybox and model draw calls)
			PushConstantData pushConstantData{};
			
			pushConstantData.modelMat = modelModelMatrix;
			pushConstantData.camDataBufferIndex = camDataBufferIndex;
			pushConstantData.skyboxCubemapIndex = skyboxTextureResourceIndex;
			pushConstantData.materialBufferIndex = model->materials[model->meshes[i]->materialIndex]->bufferIndex;
			pushConstantData.samplerIndex = samplerIndex;

			commandBuffers[currentFrame]->PushConstants(rootSig.get(), &pushConstantData);

			
			//Skybox
			commandBuffers[currentFrame]->BindPipeline(skyboxPipeline.get(), NK::PIPELINE_BIND_POINT::GRAPHICS);
			commandBuffers[currentFrame]->BindVertexBuffers(0, 1, skyboxVertBuffer.get(), &skyboxVertexBufferStride);
			commandBuffers[currentFrame]->BindIndexBuffer(skyboxIndexBuffer.get(), NK::DATA_FORMAT::R32_UINT);
			
			commandBuffers[currentFrame]->DrawIndexed(36, 1, 0, 0);
			
			
			//Model
			NK::IPipeline* pipeline{ model->materials[0]->lightingModel == NK::LIGHTING_MODEL::BLINN_PHONG ? blinnPhongPipeline.get() : pbrPipeline.get() };
			commandBuffers[currentFrame]->BindPipeline(pipeline, NK::PIPELINE_BIND_POINT::GRAPHICS);
			commandBuffers[currentFrame]->BindVertexBuffers(0, 1, model->meshes[i]->vertexBuffer.get(), &modelVertexBufferStride);
			commandBuffers[currentFrame]->BindIndexBuffer(model->meshes[i]->indexBuffer.get(), NK::DATA_FORMAT::R32_UINT);

//			float speed{ 50.0f };
//			modelModelMatrix = glm::rotate(modelModelMatrix, glm::radians(speed * static_cast<float>(NK::TimeManager::GetDeltaTime())), glm::vec3(0, 0, 1));

			commandBuffers[currentFrame]->DrawIndexed(model->meshes[i]->indexCount, 1, 0, 0);
		}

		
		commandBuffers[currentFrame]->EndRendering();

		if (enableSSAA)
		{
			//Downscaling pass
			commandBuffers[currentFrame]->TransitionBarrier(renderTarget.get(), NK::RESOURCE_STATE::RENDER_TARGET, NK::RESOURCE_STATE::COPY_SOURCE);
			commandBuffers[currentFrame]->TransitionBarrier(swapchain->GetImage(imageIndex), NK::RESOURCE_STATE::UNDEFINED, NK::RESOURCE_STATE::COPY_DEST);
			commandBuffers[currentFrame]->BlitTexture(renderTarget.get(), NK::TEXTURE_ASPECT::COLOUR, swapchain->GetImage(imageIndex), NK::TEXTURE_ASPECT::COLOUR);
			commandBuffers[currentFrame]->TransitionBarrier(swapchain->GetImage(imageIndex), NK::RESOURCE_STATE::COPY_DEST, NK::RESOURCE_STATE::PRESENT);
		}
		else
		{
			commandBuffers[currentFrame]->TransitionBarrier(swapchain->GetImage(imageIndex), NK::RESOURCE_STATE::RENDER_TARGET, NK::RESOURCE_STATE::PRESENT);
		}
		
		//Present
		commandBuffers[currentFrame]->End();


		//Submit
		graphicsQueue->Submit(commandBuffers[currentFrame].get(), imageAvailableSemaphores[currentFrame].get(), renderFinishedSemaphores[imageIndex].get(), inFlightFences[currentFrame].get());
		swapchain->Present(renderFinishedSemaphores[imageIndex].get(), imageIndex);

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	}
	camDataBuffer->Unmap();
	graphicsQueue->WaitIdle();
}