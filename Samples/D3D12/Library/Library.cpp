#include "Core/RAIIContext.h"
#include "Core/Debug/ILogger.h"
#include "Core/Memory/Allocation.h"
#include "Core/Memory/TrackingAllocator.h"
#include <Core/Utils/FormatUtils.h>
#include "RHI-D3D12/D3D12Device.h"
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

	const NK::UniquePtr<NK::IDevice> device{ NK_NEW(NK::D3D12Device, *logger, *allocator) };
	logger->Log(NK::LOGGER_CHANNEL::INFO, NK::LOGGER_LAYER::APPLICATION, "Total memory allocated: " + NK::FormatUtils::GetSizeString(dynamic_cast<NK::TrackingAllocator*>(allocator)->GetTotalMemoryAllocated()) + "\n");

	NK::CommandPoolDesc poolDesc{};
	poolDesc.type = NK::COMMAND_POOL_TYPE::GRAPHICS;
	const NK::UniquePtr<NK::ICommandPool> pool{ device->CreateCommandPool(poolDesc) };
	logger->Log(NK::LOGGER_CHANNEL::INFO, NK::LOGGER_LAYER::APPLICATION, "Total memory allocated: " + NK::FormatUtils::GetSizeString(dynamic_cast<NK::TrackingAllocator*>(allocator)->GetTotalMemoryAllocated()) + "\n");

	NK::CommandBufferDesc bufferDesc{};
	bufferDesc.level = NK::COMMAND_BUFFER_LEVEL::PRIMARY;
	const NK::UniquePtr<NK::ICommandBuffer> buffer{ pool->AllocateCommandBuffer(bufferDesc) };
	logger->Log(NK::LOGGER_CHANNEL::INFO, NK::LOGGER_LAYER::APPLICATION, "Total memory allocated: " + NK::FormatUtils::GetSizeString(dynamic_cast<NK::TrackingAllocator*>(allocator)->GetTotalMemoryAllocated()) + "\n");


	logger->Log(NK::LOGGER_CHANNEL::SUCCESS, NK::LOGGER_LAYER::APPLICATION, "Engine initialised successfully!\n");
}