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
#include <RHI/IShader.h>
#include <RHI/ISurface.h>
#include <RHI/ISwapchain.h>
#include <RHI/ITexture.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>



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
	graphicsCommandPoolDesc.type = NK::COMMAND_POOL_TYPE::GRAPHICS;
	const NK::UniquePtr<NK::ICommandPool> graphicsCommandPool{ device->CreateCommandPool(graphicsCommandPoolDesc) };

	//Primary Level Command Buffer Description
	NK::CommandBufferDesc primaryLevelCommandBufferDesc{};
	primaryLevelCommandBufferDesc.level = NK::COMMAND_BUFFER_LEVEL::PRIMARY;

	//Graphics Queue
	NK::QueueDesc graphicsQueueDesc{};
	graphicsQueueDesc.type = NK::COMMAND_POOL_TYPE::GRAPHICS;
	const NK::UniquePtr<NK::IQueue> graphicsQueue(device->CreateQueue(graphicsQueueDesc));

	//Transfer Queue
	NK::QueueDesc transferQueueDesc{};
	transferQueueDesc.type = NK::COMMAND_POOL_TYPE::TRANSFER;
	const NK::UniquePtr<NK::IQueue> transferQueue{ device->CreateQueue(transferQueueDesc) };

	//GPU Uploader
	NK::GPUUploaderDesc gpuUploaderDesc{};
	gpuUploaderDesc.stagingBufferSize = 1024 * 512 * 512; //512MiB
	gpuUploaderDesc.transferQueue = transferQueue.get();
	const NK::UniquePtr<NK::GPUUploader> gpuUploader{ device->CreateGPUUploader(gpuUploaderDesc) };

	//Window and Surface
	NK::WindowDesc windowDesc{};
	windowDesc.name = "Camera Sample";
	constexpr glm::ivec2 SCREEN_DIMENSIONS{ 1280, 720 };
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
	NK::PlayerCamera camera{ NK::PlayerCamera(glm::vec3(0,0,-3), 90.0f, 0, 0.01f, 100.0f, 90.0f, static_cast<float>(SCREEN_DIMENSIONS.x) / SCREEN_DIMENSIONS.y, 30.0f, 0.05f) };
	
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
	vertShaderDesc.filepath = "Samples/Shaders/Cube/Cube_vs";
	const NK::UniquePtr<NK::IShader> vertShader{ device->CreateShader(vertShaderDesc) };

	//Fragment Shader
	NK::ShaderDesc fragShaderDesc{};
	fragShaderDesc.type = NK::SHADER_TYPE::FRAGMENT;
	fragShaderDesc.filepath = "Samples/Shaders/Cube/Cube_fs";
	const NK::UniquePtr<NK::IShader> fragShader{ device->CreateShader(fragShaderDesc) };

	//Root Signature
	NK::RootSignatureDesc rootSigDesc{};
	rootSigDesc.num32BitPushConstantValues = 16 + 1; //model matrix + cam data buffer index
	const NK::UniquePtr<NK::IRootSignature> rootSig{ device->CreateRootSignature(rootSigDesc) };

	
	//Vertex Buffer
	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 colour;
	};
	const Vertex vertices[8] =
	{
		//Positions								//Colours
		Vertex(glm::vec3( 0.5f, -0.5f,  0.5f),	glm::vec3(1.0f, 0.0f, 0.0f)), //0: Front Bottom Right	(Red)
		Vertex(glm::vec3( 0.5f,  0.5f,  0.5f),	glm::vec3(1.0f, 1.0f, 0.0f)), //1: Front Top Right		(Yellow)
		Vertex(glm::vec3(-0.5f,  0.5f,  0.5f),	glm::vec3(0.0f, 0.0f, 1.0f)), //2: Front Top Left		(Blue)
		Vertex(glm::vec3(-0.5f, -0.5f,  0.5f),	glm::vec3(0.0f, 1.0f, 0.0f)), //3: Front Bottom Left	(Green)
		Vertex(glm::vec3( 0.5f, -0.5f, -0.5f),	glm::vec3(1.0f, 0.0f, 1.0f)), //4: Back Bottom Right	(Magenta)
		Vertex(glm::vec3( 0.5f,  0.5f, -0.5f),	glm::vec3(0.5f, 0.5f, 0.5f)), //5: Back Top Right		(Gray)
		Vertex(glm::vec3(-0.5f,  0.5f, -0.5f),	glm::vec3(1.0f, 1.0f, 1.0f)), //6: Back Top Left		(White)
		Vertex(glm::vec3(-0.5f, -0.5f, -0.5f),	glm::vec3(0.0f, 1.0f, 1.0f))  //7: Back Bottom Left		(Cyan)
	};
	
	NK::BufferDesc vertStagingBufferDesc{};
	vertStagingBufferDesc.size = sizeof(Vertex) * 8;
	vertStagingBufferDesc.type = NK::MEMORY_TYPE::HOST;
	vertStagingBufferDesc.usage = NK::BUFFER_USAGE_FLAGS::TRANSFER_SRC_BIT;
	const NK::UniquePtr<NK::IBuffer> vertStagingBuffer{ device->CreateBuffer(vertStagingBufferDesc) };
	void* vertStagingBufferMap{ vertStagingBuffer->Map() };
	memcpy(vertStagingBufferMap, vertices, sizeof(Vertex) * 8);
	vertStagingBuffer->Unmap();

	NK::BufferDesc vertBufferDesc{};
	vertBufferDesc.size = sizeof(Vertex) * 8;
	vertBufferDesc.type = NK::MEMORY_TYPE::DEVICE;
	vertBufferDesc.usage = NK::BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | NK::BUFFER_USAGE_FLAGS::VERTEX_BUFFER_BIT;
	const NK::UniquePtr<NK::IBuffer> vertBuffer{ device->CreateBuffer(vertBufferDesc) };


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
	
	NK::BufferDesc indexStagingBufferDesc{};
	indexStagingBufferDesc.size = sizeof(std::uint32_t) * 36;
	indexStagingBufferDesc.type = NK::MEMORY_TYPE::HOST;
	indexStagingBufferDesc.usage = NK::BUFFER_USAGE_FLAGS::TRANSFER_SRC_BIT;
	const NK::UniquePtr<NK::IBuffer> indexStagingBuffer{ device->CreateBuffer(indexStagingBufferDesc) };
	void* indexStagingBufferMap{ indexStagingBuffer->Map() };
	memcpy(indexStagingBufferMap, indices, sizeof(std::uint32_t) * 36);
	indexStagingBuffer->Unmap();

	NK::BufferDesc indexBufferDesc{};
	indexBufferDesc.size = sizeof(std::uint32_t) * 36;
	indexBufferDesc.type = NK::MEMORY_TYPE::DEVICE;
	indexBufferDesc.usage = NK::BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | NK::BUFFER_USAGE_FLAGS::INDEX_BUFFER_BIT;
	const NK::UniquePtr<NK::IBuffer> indexBuffer{ device->CreateBuffer(indexBufferDesc) };

	
	//Upload vertex and index buffers
	gpuUploader->EnqueueBufferDataUpload(vertices, vertBuffer.get(), NK::RESOURCE_STATE::UNDEFINED);
	gpuUploader->EnqueueBufferDataUpload(indices, indexBuffer.get(), NK::RESOURCE_STATE::UNDEFINED);

	
	//Flush gpu uploader uploads
	gpuUploader->Flush(true);
	gpuUploader->Reset();
	

	//Graphics Pipeline

	//Position attribute
	std::vector<NK::VertexAttributeDesc> vertexAttributes;
	NK::VertexAttributeDesc posAttribute{};
	posAttribute.attribute = NK::SHADER_ATTRIBUTE::POSITION;
	posAttribute.binding = 0;
	posAttribute.format = NK::DATA_FORMAT::R32G32B32_SFLOAT;
	posAttribute.offset = 0;
	vertexAttributes.push_back(posAttribute);
	NK::VertexAttributeDesc colourAttribute{};
	colourAttribute.attribute = NK::SHADER_ATTRIBUTE::COLOUR_0;
	colourAttribute.binding = 0;
	colourAttribute.format = NK::DATA_FORMAT::R32G32B32_SFLOAT;
	colourAttribute.offset = sizeof(glm::vec3);
	vertexAttributes.push_back(colourAttribute);

	//Vertex buffer binding
	std::vector<NK::VertexBufferBindingDesc> bufferBindings;
	NK::VertexBufferBindingDesc bufferBinding{};
	bufferBinding.binding = 0;
	bufferBinding.inputRate = NK::VERTEX_INPUT_RATE::VERTEX;
	bufferBinding.stride = sizeof(Vertex);
	bufferBindings.push_back(bufferBinding);

	//Vertex input description
	NK::VertexInputDesc vertexInputDesc{};
	vertexInputDesc.attributeDescriptions = vertexAttributes;
	vertexInputDesc.bufferBindingDescriptions = bufferBindings;

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
	graphicsPipelineDesc.fragmentShader = fragShader.get();
	graphicsPipelineDesc.rootSignature = rootSig.get();
	graphicsPipelineDesc.vertexInputDesc = vertexInputDesc;
	graphicsPipelineDesc.inputAssemblyDesc = inputAssemblyDesc;
	graphicsPipelineDesc.rasteriserDesc = rasteriserDesc;
	graphicsPipelineDesc.depthStencilDesc = depthStencilDesc;
	graphicsPipelineDesc.multisamplingDesc = multisamplingDesc;
	graphicsPipelineDesc.colourBlendDesc = colourBlendDesc;
	graphicsPipelineDesc.colourAttachmentFormats = { NK::DATA_FORMAT::R8G8B8A8_SRGB };
	graphicsPipelineDesc.depthStencilAttachmentFormat = NK::DATA_FORMAT::D32_SFLOAT;

	const NK::UniquePtr<NK::IPipeline> graphicsPipeline{ device->CreatePipeline(graphicsPipelineDesc) };


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
	for (std::size_t i{ 0 }; i<MAX_FRAMES_IN_FLIGHT; ++i)
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
		std::cout << currentFrame << ": ";

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
		commandBuffers[currentFrame]->BindPipeline(graphicsPipeline.get(), NK::PIPELINE_BIND_POINT::GRAPHICS);
		commandBuffers[currentFrame]->BindRootSignature(rootSig.get(), NK::PIPELINE_BIND_POINT::GRAPHICS);

		std::size_t vertexBufferStride{ sizeof(Vertex) };
		commandBuffers[currentFrame]->BindVertexBuffers(0, 1, vertBuffer.get(), &vertexBufferStride);
		commandBuffers[currentFrame]->BindIndexBuffer(indexBuffer.get(), NK::DATA_FORMAT::R32_UINT);

		commandBuffers[currentFrame]->SetViewport({ 0, 0 }, { SCREEN_DIMENSIONS.x, SCREEN_DIMENSIONS.y });
		commandBuffers[currentFrame]->SetScissor({ 0, 0 }, { SCREEN_DIMENSIONS.x, SCREEN_DIMENSIONS.y });


		//Drawing
		
		//Push constants
		struct PushConstantData
		{
			glm::mat4 modelMat;
			NK::ResourceIndex camDataBufferIndex;
		};
		PushConstantData pushConstantData{};
		pushConstantData.camDataBufferIndex = camDataBufferIndex;

		//Cube 1
		pushConstantData.modelMat = glm::mat4(1.0f);
		pushConstantData.modelMat = glm::rotate(pushConstantData.modelMat, static_cast<float>(20 * sin(glm::radians(glfwGetTime()))), glm::vec3(0,1,0));
		pushConstantData.modelMat = glm::rotate(pushConstantData.modelMat, static_cast<float>(30 * sin(glm::radians(glfwGetTime()))), glm::vec3(1,0,0));
		commandBuffers[currentFrame]->PushConstants(rootSig.get(), &pushConstantData);
		commandBuffers[currentFrame]->DrawIndexed(36, 1, 0, 0);

		//Cube 2 (basically just a background)
		pushConstantData.modelMat = glm::mat4(1.0f);
		pushConstantData.modelMat = glm::translate(pushConstantData.modelMat, glm::vec3(0,0,5));
		pushConstantData.modelMat = glm::rotate(pushConstantData.modelMat, static_cast<float>(30 * sin(glm::radians(glfwGetTime()))), glm::vec3(0,0,1));
		pushConstantData.modelMat = glm::scale(pushConstantData.modelMat, glm::vec3(10,10,1));
		commandBuffers[currentFrame]->PushConstants(rootSig.get(), &pushConstantData);
		commandBuffers[currentFrame]->DrawIndexed(36, 1, 0, 0);


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
