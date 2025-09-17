#include <queue>
#include <Core/RAIIContext.h>
#include <Core/Debug/ILogger.h>
#include <Core/Memory/Allocation.h>
#include <Core/Memory/TrackingAllocator.h>
#include <Core/Utils/FormatUtils.h>
#include <RHI-D3D12/D3D12Device.h>
#include <RHI/IBuffer.h>
#include <RHI/IShader.h>
#include <RHI/ISurface.h>
#include <RHI/ITexture.h>
#include <RHI/ISwapchain.h>

#include "RHI/IPipeline.h"
#ifdef ERROR
	#undef ERROR
#endif



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
	const NK::UniquePtr<NK::IDevice> device{ NK_NEW(NK::D3D12Device, *logger, *allocator) };

	//Surface
	NK::SurfaceDesc surfaceDesc{};
	surfaceDesc.name = "Triangle Sample";
	surfaceDesc.size = glm::ivec2(1280, 720);
	const NK::UniquePtr<NK::ISurface> surface{ device->CreateSurface(surfaceDesc) };

	//Queue
	NK::QueueDesc queueDesc{};
	queueDesc.type = NK::QUEUE_TYPE::GRAPHICS;
	const NK::UniquePtr<NK::IQueue> graphicsQueue(device->CreateQueue(queueDesc));

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


	//Graphics Pipeline
	struct Vertex
	{
		glm::vec3 pos;
	};
	//empty for now
	std::vector<NK::VertexAttributeDesc> vertexAttributes;
	std::vector<NK::VertexBufferBindingDesc> bufferBindings;
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
	graphicsPipelineDesc.colourAttachmentFormats = { NK::DATA_FORMAT::R8G8B8A8_SRGB };
	graphicsPipelineDesc.depthStencilAttachmentFormat = NK::DATA_FORMAT::UNDEFINED;

	const NK::UniquePtr<NK::IPipeline> graphicsPipeline{ device->CreatePipeline(graphicsPipelineDesc) };


	//Command Pool
	NK::CommandPoolDesc commandPoolDesc{};
	commandPoolDesc.type = NK::COMMAND_POOL_TYPE::GRAPHICS;
	const NK::UniquePtr<NK::ICommandPool> commandPool{ device->CreateCommandPool(commandPoolDesc) };

	//Command Buffer
	NK::CommandBufferDesc commandBufferDesc{};
	commandBufferDesc.level = NK::COMMAND_BUFFER_LEVEL::PRIMARY;
	const NK::UniquePtr<NK::ICommandBuffer> commandBuffer{ commandPool->AllocateCommandBuffer(commandBufferDesc) };

	//Semaphores
	const NK::UniquePtr<NK::ISemaphore> imageAvailableSemaphore{ device->CreateSemaphore() };
	const NK::UniquePtr<NK::ISemaphore> renderFinishedSemaphore{ device->CreateSemaphore() };

	//Fence
	NK::FenceDesc fenceDesc{};
	fenceDesc.initiallySignaled = true;
	const NK::UniquePtr<NK::IFence> inFlightFence{ device->CreateFence(fenceDesc) };

	
	while (!surface->ShouldClose())
	{
		std::uint32_t imageIndex{ swapchain->AcquireNextImageIndex(imageAvailableSemaphore.get(), inFlightFence.get()) };
		
		//Wait for the previous frame to finish presenting before starting a new one
		inFlightFence->Wait();
		inFlightFence->Reset();
		
		glfwPollEvents();


		commandBuffer->Begin();
		commandBuffer->TransitionBarrier(swapchain->GetImage(imageIndex), NK::RESOURCE_STATE::UNDEFINED, NK::RESOURCE_STATE::RENDER_TARGET);

		commandBuffer->BeginRendering(1, swapchain->GetImageView(imageIndex), nullptr);
		commandBuffer->BindPipeline(graphicsPipeline.get(), NK::PIPELINE_BIND_POINT::GRAPHICS);
		commandBuffer->SetViewport({ 0, 0 }, { 1280, 720 });
		commandBuffer->SetScissor({ 0, 0 }, { 1280, 720 });
		commandBuffer->Draw(3, 1, 0, 0);
		commandBuffer->EndRendering();

		commandBuffer->TransitionBarrier(swapchain->GetImage(imageIndex), NK::RESOURCE_STATE::RENDER_TARGET, NK::RESOURCE_STATE::PRESENT);
		commandBuffer->End();

		graphicsQueue->Submit(commandBuffer.get(), imageAvailableSemaphore.get(), renderFinishedSemaphore.get(), inFlightFence.get());
		swapchain->Present(renderFinishedSemaphore.get(), imageIndex);

		//todo: very VERY sloppy temporary workaround - need to figure out how to do all this synchronisation stuff properly
		graphicsQueue->WaitIdle();
	}
}