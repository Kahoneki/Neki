#include <cstring>
#include <Core/RAIIContext.h>
#include <Core/Debug/ILogger.h>
#include <Core/Memory/Allocation.h>
#include <Core/Memory/TrackingAllocator.h>
#include <Core/Utils/FormatUtils.h>
#include <RHI-Vulkan/VulkanDevice.h>
#include <RHI/IBuffer.h>
#include <RHI/IPipeline.h>
#include <RHI/ISampler.h>
#include <RHI/IShader.h>
#include <RHI/ISurface.h>
#include <RHI/ISwapchain.h>
#include <RHI/ITexture.h>

#include "Core/Utils/ImageLoader.h"
#include "Graphics/GPUUploader.h"



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

	//GPU Uploader
	const NK::UniquePtr<NK::GPUUploader> gpuUploader{ device->CreateGPUUploader(1024 * 1024 * 512) };

	//Surface
	NK::SurfaceDesc surfaceDesc{};
	surfaceDesc.name = "Texture Sample";
	surfaceDesc.size = glm::ivec2(1280, 720);
	const NK::UniquePtr<NK::ISurface> surface{ device->CreateSurface(surfaceDesc) };

	//Swapchain
	NK::SwapchainDesc swapchainDesc{};
	swapchainDesc.surface = surface.get();
	swapchainDesc.numBuffers = 3;
	swapchainDesc.presentQueue = graphicsQueue.get();
	const NK::UniquePtr<NK::ISwapchain> swapchain{ device->CreateSwapchain(swapchainDesc) };

	
	//Texture
	NK::ImageData imageData{ NK::ImageLoader::LoadImage("Samples/Resource Files/NekiLogo.png", true, true) };
	imageData.desc.usage |= NK::TEXTURE_USAGE_FLAGS::READ_ONLY;
	const NK::UniquePtr<NK::ITexture> texture{ device->CreateTexture(imageData.desc) };
	gpuUploader->EnqueueTextureDataUpload(imageData.numBytes, imageData.data, texture.get(), NK::RESOURCE_STATE::UNDEFINED);

	
	//Texture view
	NK::TextureViewDesc textureViewDesc{};
	textureViewDesc.dimension = NK::TEXTURE_DIMENSION::DIM_2;
	textureViewDesc.format = NK::DATA_FORMAT::R8G8B8A8_SRGB;
	textureViewDesc.type = NK::TEXTURE_VIEW_TYPE::SHADER_READ_ONLY;
	const NK::UniquePtr<NK::ITextureView> textureView{ device->CreateTextureView(texture.get(), textureViewDesc) };
	NK::ResourceIndex textureResourceIndex{ textureView->GetIndex() };

	//Sampler
	NK::SamplerDesc samplerDesc{};
	samplerDesc.minFilter = NK::FILTER_MODE::NEAREST;
	samplerDesc.magFilter = NK::FILTER_MODE::NEAREST;
	const NK::UniquePtr<NK::ISampler> sampler{ device->CreateSampler(samplerDesc) };
	NK::SamplerIndex samplerIndex{ sampler->GetIndex() };
	

	//Vertex Shader
	NK::ShaderDesc vertShaderDesc{};
	vertShaderDesc.type = NK::SHADER_TYPE::VERTEX;
	vertShaderDesc.filepath = "Samples/Shaders/Texture/Texture_vs";
	const NK::UniquePtr<NK::IShader> vertShader{ device->CreateShader(vertShaderDesc) };

	//Fragment Shader
	NK::ShaderDesc fragShaderDesc{};
	fragShaderDesc.type = NK::SHADER_TYPE::FRAGMENT;
	fragShaderDesc.filepath = "Samples/Shaders/Texture/Texture_fs";
	const NK::UniquePtr<NK::IShader> fragShader{ device->CreateShader(fragShaderDesc) };

	//Root Signature
	NK::RootSignatureDesc rootSigDesc{};
	rootSigDesc.num32BitPushConstantValues = 2;
	const NK::UniquePtr<NK::IRootSignature> rootSig{ device->CreateRootSignature(rootSigDesc) };

	
	//Vertex Buffer
	struct Vertex
	{
		glm::vec2 pos;
		glm::vec2 texCoord;
	};
	const Vertex vertices[4]
	{
		Vertex(glm::vec2(-0.5f, -0.5f),	glm::vec2(0.0f, 0.0f)),	//Bottom left
		Vertex(glm::vec2(-0.5f, 0.5f),	glm::vec2(0.0f, 1.0f)),	//Top left
		Vertex(glm::vec2(0.5f, -0.5f),	glm::vec2(1.0f, 0.0f)),	//Bottom right
		Vertex(glm::vec2(0.5f, 0.5f),	glm::vec2(1.0f, 1.0f))	//Top right
	};
	NK::BufferDesc vertBufferDesc{};
	vertBufferDesc.size = sizeof(Vertex) * 4;
	vertBufferDesc.type = NK::MEMORY_TYPE::DEVICE;
	vertBufferDesc.usage = NK::BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | NK::BUFFER_USAGE_FLAGS::VERTEX_BUFFER_BIT;
	const NK::UniquePtr<NK::IBuffer> vertBuffer{ device->CreateBuffer(vertBufferDesc) };

	//Index Buffer
	const std::uint32_t indices[6]{ 0, 1, 2, 2, 1, 3 };
	NK::BufferDesc indexBufferDesc{};
	indexBufferDesc.size = sizeof(std::uint32_t) * 6;
	indexBufferDesc.type = NK::MEMORY_TYPE::DEVICE;
	indexBufferDesc.usage = NK::BUFFER_USAGE_FLAGS::TRANSFER_DST_BIT | NK::BUFFER_USAGE_FLAGS::INDEX_BUFFER_BIT;
	const NK::UniquePtr<NK::IBuffer> indexBuffer{ device->CreateBuffer(indexBufferDesc) };

	//Upload vertex and index buffers
	gpuUploader->EnqueueBufferDataUpload(sizeof(Vertex) * 4, vertices, vertBuffer.get(), NK::RESOURCE_STATE::UNDEFINED);
	gpuUploader->EnqueueBufferDataUpload(sizeof(std::uint32_t) * 6, indices, indexBuffer.get(), NK::RESOURCE_STATE::UNDEFINED);

	//Flush texture, vertex buffer, and index buffer
	gpuUploader->Flush(true);
	gpuUploader->Reset();
	

	//Graphics Pipeline

	std::vector<NK::VertexAttributeDesc> vertexAttributes;
	//Position attribute
	NK::VertexAttributeDesc posAttribute{};
	posAttribute.attribute = NK::SHADER_ATTRIBUTE::POSITION;
	posAttribute.binding = 0;
	posAttribute.format = NK::DATA_FORMAT::R32G32_SFLOAT;
	posAttribute.offset = 0;
	vertexAttributes.push_back(posAttribute);
	//Texcoord attribute
	NK::VertexAttributeDesc uvAttribute{};
	uvAttribute.attribute = NK::SHADER_ATTRIBUTE::TEXCOORD_0;
	uvAttribute.binding = 0;
	uvAttribute.format = NK::DATA_FORMAT::R32G32_SFLOAT;
	uvAttribute.offset = sizeof(glm::vec2);
	vertexAttributes.push_back(uvAttribute);

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
	colourBlendAttachments[0].blendEnable = true;
	colourBlendAttachments[0].srcColourBlendFactor = NK::BLEND_FACTOR::SRC_ALPHA;
	colourBlendAttachments[0].dstColourBlendFactor = NK::BLEND_FACTOR::ONE_MINUS_SRC_ALPHA;
	colourBlendAttachments[0].colourBlendOp = NK::BLEND_OP::ADD;
	colourBlendAttachments[0].srcAlphaBlendFactor = NK::BLEND_FACTOR::ONE; //Keep source alpha
	colourBlendAttachments[0].dstAlphaBlendFactor = NK::BLEND_FACTOR::ZERO; //Discard destination alpha
	colourBlendAttachments[0].alphaBlendOp = NK::BLEND_OP::ADD;
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
	//Transition texture
	commandBuffers[0]->Begin();
	commandBuffers[0]->TransitionBarrier(texture.get(), NK::RESOURCE_STATE::COPY_DEST, NK::RESOURCE_STATE::SHADER_RESOURCE);
	commandBuffers[0]->End();
	graphicsQueue->Submit(commandBuffers[0].get(), nullptr, nullptr, nullptr);
	graphicsQueue->WaitIdle();

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
		commandBuffers[currentFrame]->BindRootSignature(rootSig.get(), NK::PIPELINE_BIND_POINT::GRAPHICS);
		std::uint32_t pushConstants[]{ textureResourceIndex, samplerIndex };
		commandBuffers[currentFrame]->PushConstants(rootSig.get(), pushConstants);

		std::size_t vertexBufferStride{ sizeof(Vertex) };
		commandBuffers[currentFrame]->BindVertexBuffers(0, 1, vertBuffer.get(), &vertexBufferStride);
		commandBuffers[currentFrame]->BindIndexBuffer(indexBuffer.get(), NK::DATA_FORMAT::R32_UINT);

		commandBuffers[currentFrame]->SetViewport({ 0, 0 }, { 1280, 720 });
		commandBuffers[currentFrame]->SetScissor({ 0, 0 }, { 1280, 720 });
		commandBuffers[currentFrame]->DrawIndexed(6, 1, 0, 0);
		commandBuffers[currentFrame]->EndRendering();

		commandBuffers[currentFrame]->TransitionBarrier(swapchain->GetImage(imageIndex), NK::RESOURCE_STATE::RENDER_TARGET, NK::RESOURCE_STATE::PRESENT);
		commandBuffers[currentFrame]->End();

		graphicsQueue->Submit(commandBuffers[currentFrame].get(), imageAvailableSemaphores[currentFrame].get(), renderFinishedSemaphores[imageIndex].get(), inFlightFences[currentFrame].get());
		swapchain->Present(renderFinishedSemaphores[imageIndex].get(), imageIndex);

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	}
	graphicsQueue->WaitIdle();
}