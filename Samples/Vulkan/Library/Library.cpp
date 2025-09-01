#include "Core/RAIIContext.h"
#include "Core/Debug/ILogger.h"
#include "Core/Memory/Allocation.h"
#include "Core/Memory/TrackingAllocator.h"
#include "Core/Utils/FormatUtils.h"
#include "RHI-Vulkan/VulkanDevice.h"
#include "RHI/IBuffer.h"
#include "RHI/ITexture.h"
#include "RHI/ICommandBuffer.h"
#include "RHI/ICommandPool.h"
#include "RHI/IDevice.h"



int main()
{
	NK::LoggerConfig loggerConfig{ NK::LOGGER_TYPE::CONSOLE, true };
	loggerConfig.SetLayerChannelBitfield(NK::LOGGER_LAYER::VULKAN_GENERAL, NK::LOGGER_CHANNEL::NONE);
	loggerConfig.SetLayerChannelBitfield(NK::LOGGER_LAYER::VULKAN_VALIDATION, NK::LOGGER_CHANNEL::NONE);
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
	bufferViewDesc.type = NK::BUFFER_VIEW_TYPE::CONSTANT;
	bufferViewDesc.offset = 0;
	bufferViewDesc.size = bufferDesc.size;
	const NK::ResourceIndex bufferViewIndex{ device->CreateBufferView(buffer.get(), bufferViewDesc) };
	logger->Log(NK::LOGGER_CHANNEL::INFO, NK::LOGGER_LAYER::APPLICATION, "Buffer view index: " + std::to_string(bufferViewIndex) + "\n");
	logger->Log(NK::LOGGER_CHANNEL::INFO, NK::LOGGER_LAYER::APPLICATION, "Total memory allocated: " + NK::FormatUtils::GetSizeString(dynamic_cast<NK::TrackingAllocator*>(allocator)->GetTotalMemoryAllocated()) + "\n\n");
	
	NK::TextureDesc textureDesc{};
	textureDesc.size = glm::ivec3(1920, 1080, 1);
	textureDesc.arrayTexture = false;
	textureDesc.usage = NK::TEXTURE_USAGE_FLAGS::COLOUR_ATTACHMENT;
	textureDesc.format = NK::TEXTURE_FORMAT::R8G8B8A8_SRGB;
	textureDesc.dimension = NK::TEXTURE_DIMENSION::DIM_2;
	const NK::UniquePtr<NK::ITexture> texture{ device->CreateTexture(textureDesc) };
	logger->Log(NK::LOGGER_CHANNEL::INFO, NK::LOGGER_LAYER::APPLICATION, "Total memory allocated: " + NK::FormatUtils::GetSizeString(dynamic_cast<NK::TrackingAllocator*>(allocator)->GetTotalMemoryAllocated()) + "\n\n");

	NK::SurfaceDesc surfaceDesc{};
	surfaceDesc.name = "Neki-Vulkan Library Sample";
	surfaceDesc.size = glm::ivec2(1280, 720);
	const NK::UniquePtr<NK::ISurface> surface{ device->CreateSurface(surfaceDesc) };
	logger->Log(NK::LOGGER_CHANNEL::INFO, NK::LOGGER_LAYER::APPLICATION, "Total memory allocated: " + NK::FormatUtils::GetSizeString(dynamic_cast<NK::TrackingAllocator*>(allocator)->GetTotalMemoryAllocated()) + "\n\n");
	
	logger->Log(NK::LOGGER_CHANNEL::SUCCESS, NK::LOGGER_LAYER::APPLICATION, "Engine initialised successfully!\n\n");
}