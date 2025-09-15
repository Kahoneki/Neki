#include <Core/RAIIContext.h>
#include <Core/Debug/ILogger.h>
#include <Core/Memory/Allocation.h>
#include <Core/Memory/TrackingAllocator.h>
#include <Core/Utils/FormatUtils.h>
#include <RHI-Vulkan/VulkanDevice.h>
#include <RHI/IBuffer.h>
#include <RHI/IBufferView.h>
#include <RHI/ICommandBuffer.h>
#include <RHI/ICommandPool.h>
#include <RHI/IPipeline.h>
#include <RHI/IShader.h>
#include <RHI/ISurface.h>
#include <RHI/ISwapchain.h>
#include <RHI/ITexture.h>


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
	
	const NK::UniquePtr<NK::IDevice> device{ NK_NEW(NK::VulkanDevice, *logger, *allocator) };
	logger->Log(NK::LOGGER_CHANNEL::INFO, NK::LOGGER_LAYER::APPLICATION, "Total memory allocated: " + NK::FormatUtils::GetSizeString(dynamic_cast<NK::TrackingAllocator*>(allocator)->GetTotalMemoryAllocated()) + "\n\n");

	NK::CommandPoolDesc poolDesc{};
	poolDesc.type = NK::COMMAND_POOL_TYPE::GRAPHICS;
	const NK::UniquePtr<NK::ICommandPool> pool{ device->CreateCommandPool(poolDesc) };
	logger->Log(NK::LOGGER_CHANNEL::INFO, NK::LOGGER_LAYER::APPLICATION, "Total memory allocated: " + NK::FormatUtils::GetSizeString(dynamic_cast<NK::TrackingAllocator*>(allocator)->GetTotalMemoryAllocated()) + "\n\n");

	NK::CommandBufferDesc commandBufferDesc{};
	commandBufferDesc.level = NK::COMMAND_BUFFER_LEVEL::PRIMARY;
	const NK::UniquePtr<NK::ICommandBuffer> commandBuffer{ pool->AllocateCommandBuffer(commandBufferDesc) };
	logger->Log(NK::LOGGER_CHANNEL::INFO, NK::LOGGER_LAYER::APPLICATION, "Total memory allocated: " + NK::FormatUtils::GetSizeString(dynamic_cast<NK::TrackingAllocator*>(allocator)->GetTotalMemoryAllocated()) + "\n\n");
	
	NK::BufferDesc bufferDesc{};
	bufferDesc.size = 1024;
	bufferDesc.type = NK::MEMORY_TYPE::DEVICE;
	bufferDesc.usage = NK::BUFFER_USAGE_FLAGS::UNIFORM_BUFFER_BIT;
	const NK::UniquePtr<NK::IBuffer> buffer{ device->CreateBuffer(bufferDesc) };
	logger->Log(NK::LOGGER_CHANNEL::INFO, NK::LOGGER_LAYER::APPLICATION, "Total memory allocated: " + NK::FormatUtils::GetSizeString(dynamic_cast<NK::TrackingAllocator*>(allocator)->GetTotalMemoryAllocated()) + "\n\n");

	NK::BufferViewDesc bufferViewDesc{};
	bufferViewDesc.type = NK::BUFFER_VIEW_TYPE::UNIFORM;
	bufferViewDesc.offset = 0;
	bufferViewDesc.size = bufferDesc.size;
	NK::UniquePtr<NK::IBufferView> bufferView{ device->CreateBufferView(buffer.get(), bufferViewDesc) };
	const NK::ResourceIndex bufferViewIndex{ bufferView->GetIndex() };
	logger->Log(NK::LOGGER_CHANNEL::INFO, NK::LOGGER_LAYER::APPLICATION, "Buffer view index: " + std::to_string(bufferViewIndex) + "\n");
	logger->Log(NK::LOGGER_CHANNEL::INFO, NK::LOGGER_LAYER::APPLICATION, "Total memory allocated: " + NK::FormatUtils::GetSizeString(dynamic_cast<NK::TrackingAllocator*>(allocator)->GetTotalMemoryAllocated()) + "\n\n");
	
	NK::TextureDesc textureDesc{};
	textureDesc.size = glm::ivec3(400, 400, 1);
	textureDesc.arrayTexture = false;
	textureDesc.usage = NK::TEXTURE_USAGE_FLAGS::READ_ONLY;
	textureDesc.format = NK::DATA_FORMAT::R8G8B8A8_SRGB;
	textureDesc.dimension = NK::TEXTURE_DIMENSION::DIM_2;
	const NK::UniquePtr<NK::ITexture> texture{ device->CreateTexture(textureDesc) };
	logger->Log(NK::LOGGER_CHANNEL::INFO, NK::LOGGER_LAYER::APPLICATION, "Total memory allocated: " + NK::FormatUtils::GetSizeString(dynamic_cast<NK::TrackingAllocator*>(allocator)->GetTotalMemoryAllocated()) + "\n\n");

	NK::TextureViewDesc textureViewDesc{};
	textureViewDesc.dimension = NK::TEXTURE_DIMENSION::DIM_2;
	textureViewDesc.format = textureDesc.format;
	textureViewDesc.type = NK::TEXTURE_VIEW_TYPE::SHADER_READ_ONLY;
	const NK::UniquePtr<NK::ITextureView> textureView{ device->CreateTextureView(texture.get(), textureViewDesc) };
	logger->Log(NK::LOGGER_CHANNEL::INFO, NK::LOGGER_LAYER::APPLICATION, "Total memory allocated: " + NK::FormatUtils::GetSizeString(dynamic_cast<NK::TrackingAllocator*>(allocator)->GetTotalMemoryAllocated()) + "\n\n");
	
	NK::SurfaceDesc surfaceDesc{};
	surfaceDesc.name = "Neki-Vulkan Library Sample";
	surfaceDesc.size = glm::ivec2(1280, 720);
	const NK::UniquePtr<NK::ISurface> surface{ device->CreateSurface(surfaceDesc) };
	logger->Log(NK::LOGGER_CHANNEL::INFO, NK::LOGGER_LAYER::APPLICATION, "Total memory allocated: " + NK::FormatUtils::GetSizeString(dynamic_cast<NK::TrackingAllocator*>(allocator)->GetTotalMemoryAllocated()) + "\n\n");

	NK::SwapchainDesc swapchainDesc{};
	swapchainDesc.surface = surface.get();
	swapchainDesc.numBuffers = 3;
	const NK::UniquePtr<NK::ISwapchain> swapchain{ device->CreateSwapchain(swapchainDesc) };
	logger->Log(NK::LOGGER_CHANNEL::INFO, NK::LOGGER_LAYER::APPLICATION, "Total memory allocated: " + NK::FormatUtils::GetSizeString(dynamic_cast<NK::TrackingAllocator*>(allocator)->GetTotalMemoryAllocated()) + "\n\n");

	NK::ShaderDesc vertShaderDesc{};
	vertShaderDesc.type = NK::SHADER_TYPE::VERTEX;
	vertShaderDesc.filepath = "Samples/Shaders/Library_vs";
	const NK::UniquePtr<NK::IShader> vertShader{ device->CreateShader(vertShaderDesc) };
	logger->Log(NK::LOGGER_CHANNEL::INFO, NK::LOGGER_LAYER::APPLICATION, "Total memory allocated: " + NK::FormatUtils::GetSizeString(dynamic_cast<NK::TrackingAllocator*>(allocator)->GetTotalMemoryAllocated()) + "\n\n");

	NK::ShaderDesc fragShaderDesc{};
	fragShaderDesc.type = NK::SHADER_TYPE::FRAGMENT;
	fragShaderDesc.filepath = "Samples/Shaders/Library_fs";
	const NK::UniquePtr<NK::IShader> fragShader{ device->CreateShader(fragShaderDesc) };
	logger->Log(NK::LOGGER_CHANNEL::INFO, NK::LOGGER_LAYER::APPLICATION, "Total memory allocated: " + NK::FormatUtils::GetSizeString(dynamic_cast<NK::TrackingAllocator*>(allocator)->GetTotalMemoryAllocated()) + "\n\n");

	NK::ShaderDesc compShaderDesc{};
	compShaderDesc.type = NK::SHADER_TYPE::COMPUTE;
	compShaderDesc.filepath = "Samples/Shaders/Library_cs";
	const NK::UniquePtr<NK::IShader> compShader{ device->CreateShader(compShaderDesc) };
	logger->Log(NK::LOGGER_CHANNEL::INFO, NK::LOGGER_LAYER::APPLICATION, "Total memory allocated: " + NK::FormatUtils::GetSizeString(dynamic_cast<NK::TrackingAllocator*>(allocator)->GetTotalMemoryAllocated()) + "\n\n");
	

	//Graphics Pipeline
	struct Vertex { glm::vec3 pos; };
	std::vector<NK::VertexAttributeDesc> vertexAttributes(1);
	vertexAttributes[0].binding = 0;
	vertexAttributes[0].attribute = NK::SHADER_ATTRIBUTE::POSITION;
	vertexAttributes[0].format = NK::DATA_FORMAT::R32_SFLOAT;
	vertexAttributes[0].offset = offsetof(Vertex, pos);
	std::vector<NK::VertexBufferBindingDesc> bufferBindings(1);
	bufferBindings[0].binding = 0;
	bufferBindings[0].stride = sizeof(Vertex);
	bufferBindings[0].inputRate = NK::VERTEX_INPUT_RATE::VERTEX;
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
	graphicsPipelineDesc.vertexInputDesc = vertexInputDesc;
	graphicsPipelineDesc.inputAssemblyDesc = inputAssemblyDesc;
	graphicsPipelineDesc.rasteriserDesc = rasteriserDesc;
	graphicsPipelineDesc.depthStencilDesc = depthStencilDesc;
	graphicsPipelineDesc.multisamplingDesc = multisamplingDesc;
	graphicsPipelineDesc.colourBlendDesc = colourBlendDesc;
	graphicsPipelineDesc.colourAttachmentFormats = { NK::DATA_FORMAT::R8G8B8A8_SRGB };
	graphicsPipelineDesc.depthStencilAttachmentFormat = NK::DATA_FORMAT::D24_UNORM_S8_UINT;

	const NK::UniquePtr<NK::IPipeline> graphicsPipeline{ device->CreatePipeline(graphicsPipelineDesc) };
	logger->Log(NK::LOGGER_CHANNEL::INFO, NK::LOGGER_LAYER::APPLICATION, "Total memory allocated: " + NK::FormatUtils::GetSizeString(dynamic_cast<NK::TrackingAllocator*>(allocator)->GetTotalMemoryAllocated()) + "\n\n");
	
	
	//Compute pipeline
	NK::PipelineDesc computePipelineDesc{};
	computePipelineDesc.type = NK::PIPELINE_TYPE::COMPUTE;
	computePipelineDesc.computeShader = compShader.get();
	const NK::UniquePtr<NK::IPipeline> computePipeline{ device->CreatePipeline(computePipelineDesc) };
	logger->Log(NK::LOGGER_CHANNEL::INFO, NK::LOGGER_LAYER::APPLICATION, "Total memory allocated: " + NK::FormatUtils::GetSizeString(dynamic_cast<NK::TrackingAllocator*>(allocator)->GetTotalMemoryAllocated()) + "\n\n");
	
	
	logger->Log(NK::LOGGER_CHANNEL::SUCCESS, NK::LOGGER_LAYER::APPLICATION, "Engine initialised successfully!\n\n");
}