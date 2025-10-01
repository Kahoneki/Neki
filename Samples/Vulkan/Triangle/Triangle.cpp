#include <cstring>
#include <Core/RAIIContext.h>
#include <Core/Debug/ILogger.h>
#include <Core/Memory/Allocation.h>
#include <Core/Memory/TrackingAllocator.h>
#include <Core/Utils/FormatUtils.h>
#include <RHI-Vulkan/VulkanDevice.h>
#include <RHI/IBuffer.h>
#include <RHI/IShader.h>
#include <RHI/ISurface.h>
#include <RHI/ITexture.h>
#include <RHI/ISwapchain.h>
#include "RHI/IPipeline.h"



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

	//Transfer Command Pool
	NK::CommandPoolDesc transferCommandPoolDesc{};
	transferCommandPoolDesc.type = NK::COMMAND_POOL_TYPE::TRANSFER;
	const NK::UniquePtr<NK::ICommandPool> transferCommandPool{ device->CreateCommandPool(transferCommandPoolDesc) };

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
	const NK::UniquePtr<NK::IQueue> transferQueue(device->CreateQueue(transferQueueDesc));

	//Surface
	NK::SurfaceDesc surfaceDesc{};
	surfaceDesc.name = "Triangle Sample";
	surfaceDesc.size = glm::ivec2(1280, 720);
	const NK::UniquePtr<NK::ISurface> surface{ device->CreateSurface(surfaceDesc) };

	//Swapchain
	NK::SwapchainDesc swapchainDesc{};
	swapchainDesc.surface = surface.get();
	swapchainDesc.numBuffers = 3;
	swapchainDesc.presentQueue = graphicsQueue.get();
	const NK::UniquePtr<NK::ISwapchain> swapchain{ device->CreateSwapchain(swapchainDesc) };

	//Vertex Shader
	NK::ShaderDesc vertShaderDesc{};
	vertShaderDesc.type = NK::SHADER_TYPE::VERTEX;
	vertShaderDesc.filepath = "Samples/Shaders/Triangle/Triangle_vs";
	const NK::UniquePtr<NK::IShader> vertShader{ device->CreateShader(vertShaderDesc) };

	//Fragment Shader
	NK::ShaderDesc fragShaderDesc{};
	fragShaderDesc.type = NK::SHADER_TYPE::FRAGMENT;
	fragShaderDesc.filepath = "Samples/Shaders/Triangle/Triangle_fs";
	const NK::UniquePtr<NK::IShader> fragShader{ device->CreateShader(fragShaderDesc) };

	//Transfer Command Buffer
	const NK::UniquePtr<NK::ICommandBuffer> transferCommandBuffer{ transferCommandPool->AllocateCommandBuffer(primaryLevelCommandBufferDesc) };

	
	//Vertex Buffer
	struct Vertex
	{
		glm::vec2 pos;
		glm::vec3 colour;
	};
	const Vertex vertices[3]
	{
		Vertex(glm::vec2(0.0f, 0.5f), glm::vec3(1.0f, 0.0f, 0.0f)),  //Top center
		Vertex(glm::vec2(0.5f, -0.5f),  glm::vec3(0.0f, 1.0f, 0.0f)), //Bottom right
		Vertex(glm::vec2(-0.5f, -0.5f), glm::vec3(0.0f, 0.0f, 1.0f)) //Bottom left
	};
	
	NK::BufferDesc vertStagingBufferDesc{};
	vertStagingBufferDesc.size = sizeof(Vertex) * 3;
	vertStagingBufferDesc.type = NK::MEMORY_TYPE::HOST;
	vertStagingBufferDesc.usage = NK::BUFFER_USAGE_FLAGS::TRANSFER_SRC_BIT;
	const NK::UniquePtr<NK::IBuffer> vertStagingBuffer{ device->CreateBuffer(vertStagingBufferDesc) };
	void* vertStagingBufferMap{ vertStagingBuffer->Map() };
	memcpy(vertStagingBufferMap, vertices, sizeof(Vertex) * 3);
	vertStagingBuffer->Unmap();

	NK::BufferDesc vertBufferDesc{};
	vertBufferDesc.size = sizeof(Vertex) * 3;
	vertBufferDesc.type = NK::MEMORY_TYPE::DEVICE;
	vertBufferDesc.usage = NK::BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | NK::BUFFER_USAGE_FLAGS::VERTEX_BUFFER_BIT;
	const NK::UniquePtr<NK::IBuffer> vertBuffer{ device->CreateBuffer(vertBufferDesc) };


	//Index Buffer
	const std::uint32_t indices[3]{ 0, 1, 2 };
	
	NK::BufferDesc indexStagingBufferDesc{};
	indexStagingBufferDesc.size = sizeof(std::uint32_t) * 3;
	indexStagingBufferDesc.type = NK::MEMORY_TYPE::HOST;
	indexStagingBufferDesc.usage = NK::BUFFER_USAGE_FLAGS::TRANSFER_SRC_BIT;
	const NK::UniquePtr<NK::IBuffer> indexStagingBuffer{ device->CreateBuffer(indexStagingBufferDesc) };
	void* indexStagingBufferMap{ indexStagingBuffer->Map() };
	memcpy(indexStagingBufferMap, indices, sizeof(std::uint32_t) * 3);
	indexStagingBuffer->Unmap();

	NK::BufferDesc indexBufferDesc{};
	indexBufferDesc.size = sizeof(std::uint32_t) * 3;
	indexBufferDesc.type = NK::MEMORY_TYPE::DEVICE;
	indexBufferDesc.usage = NK::BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | NK::BUFFER_USAGE_FLAGS::INDEX_BUFFER_BIT;
	const NK::UniquePtr<NK::IBuffer> indexBuffer{ device->CreateBuffer(indexBufferDesc) };


	//Copy Vertex and Index Buffers
	transferCommandBuffer->Begin();
	transferCommandBuffer->CopyBuffer(vertStagingBuffer.get(), vertBuffer.get());
	transferCommandBuffer->CopyBuffer(indexStagingBuffer.get(), indexBuffer.get());
	transferCommandBuffer->End();
	transferQueue->Submit(transferCommandBuffer.get(), nullptr, nullptr, nullptr);
	transferQueue->WaitIdle();
	

	//Graphics Pipeline

	//Position attribute
	std::vector<NK::VertexAttributeDesc> vertexAttributes;
	NK::VertexAttributeDesc posAttribute{};
	posAttribute.attribute = NK::SHADER_ATTRIBUTE::POSITION;
	posAttribute.binding = 0;
	posAttribute.format = NK::DATA_FORMAT::R32G32_SFLOAT;
	posAttribute.offset = 0;
	vertexAttributes.push_back(posAttribute);
	NK::VertexAttributeDesc colourAttribute{};
	colourAttribute.attribute = NK::SHADER_ATTRIBUTE::COLOUR_0;
	colourAttribute.binding = 0;
	colourAttribute.format = NK::DATA_FORMAT::R32G32B32_SFLOAT;
	colourAttribute.offset = sizeof(glm::vec2);
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
	depthStencilDesc.depthTestEnable = false;
	depthStencilDesc.depthWriteEnable = false;
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
	graphicsPipelineDesc.vertexInputDesc = vertexInputDesc;
	graphicsPipelineDesc.inputAssemblyDesc = inputAssemblyDesc;
	graphicsPipelineDesc.rasteriserDesc = rasteriserDesc;
	graphicsPipelineDesc.depthStencilDesc = depthStencilDesc;
	graphicsPipelineDesc.multisamplingDesc = multisamplingDesc;
	graphicsPipelineDesc.colourBlendDesc = colourBlendDesc;
	graphicsPipelineDesc.colourAttachmentFormats = { NK::DATA_FORMAT::R8G8B8A8_UNORM };
	graphicsPipelineDesc.depthStencilAttachmentFormat = NK::DATA_FORMAT::UNDEFINED;

	const NK::UniquePtr<NK::IPipeline> graphicsPipeline{ device->CreatePipeline(graphicsPipelineDesc) };


	//Number of frames the CPU can get ahead of the GPU
	constexpr std::uint32_t MAX_FRAMES_IN_FLIGHT{ 2 };

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
	
	while (!surface->ShouldClose())
	{
		inFlightFences[currentFrame]->Wait();
		inFlightFences[currentFrame]->Reset();

		std::uint32_t imageIndex{ swapchain->AcquireNextImageIndex(imageAvailableSemaphores[currentFrame].get(), nullptr) };

		glfwPollEvents();
		commandBuffers[currentFrame]->Begin();
		commandBuffers[currentFrame]->TransitionBarrier(swapchain->GetImage(imageIndex), NK::RESOURCE_STATE::UNDEFINED, NK::RESOURCE_STATE::RENDER_TARGET);

		commandBuffers[currentFrame]->BeginRendering(1, swapchain->GetImageView(imageIndex), nullptr);
		commandBuffers[currentFrame]->BindPipeline(graphicsPipeline.get(), NK::PIPELINE_BIND_POINT::GRAPHICS);
		commandBuffers[currentFrame]->BindDescriptorSet(device->GetDescriptorSet(), NK::PIPELINE_BIND_POINT::GRAPHICS);

		std::size_t vertexBufferStride{ sizeof(Vertex) };
		commandBuffers[currentFrame]->BindVertexBuffers(0, 1, vertBuffer.get(), &vertexBufferStride);
		commandBuffers[currentFrame]->BindIndexBuffer(indexBuffer.get(), NK::DATA_FORMAT::R32_UINT);

		commandBuffers[currentFrame]->SetViewport({ 0, 0 }, { 1280, 720 });
		commandBuffers[currentFrame]->SetScissor({ 0, 0 }, { 1280, 720 });
		commandBuffers[currentFrame]->DrawIndexed(3, 1, 0, 0);
		commandBuffers[currentFrame]->EndRendering();

		commandBuffers[currentFrame]->TransitionBarrier(swapchain->GetImage(imageIndex), NK::RESOURCE_STATE::RENDER_TARGET, NK::RESOURCE_STATE::PRESENT);
		commandBuffers[currentFrame]->End();

		graphicsQueue->Submit(commandBuffers[currentFrame].get(), imageAvailableSemaphores[currentFrame].get(), renderFinishedSemaphores[imageIndex].get(), inFlightFences[currentFrame].get());
		swapchain->Present(renderFinishedSemaphores[imageIndex].get(), imageIndex);

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	}
	graphicsQueue->WaitIdle();
}