#include "Core/Context.h"
#include "Core/Debug/ILogger.h"
#include "Core/Memory/Allocation.h"
#include "Core/Memory/TrackingAllocator.h"
#include "Core/Utils/FormatUtils.h"
#include "RHI-Vulkan/VulkanDevice.h"
#include "RHI/IDevice.h"



int main()
{
	NK::LoggerConfig loggerConfig{ NK::LOGGER_TYPE::CONSOLE, true };
	loggerConfig.SetLayerChannelBitfield(NK::LOGGER_LAYER::VULKAN_GENERAL, NK::LOGGER_CHANNEL::NONE);
	loggerConfig.SetLayerChannelBitfield(NK::LOGGER_LAYER::VULKAN_VALIDATION, NK::LOGGER_CHANNEL::NONE);
	NK::Context::Initialise(loggerConfig, NK::ALLOCATOR_TYPE::TRACKING_VERBOSE);
	NK::ILogger* logger{ NK::Context::GetLogger() };
	NK::IAllocator* allocator{ NK::Context::GetAllocator() };
	
	NK::IDevice* device{ NK_NEW(NK::VulkanDevice, *logger, *allocator) };

	logger->Log(NK::LOGGER_CHANNEL::INFO, NK::LOGGER_LAYER::APPLICATION, "Total memory allocated: " + NK::FormatUtils::GetSizeString(dynamic_cast<NK::TrackingAllocator*>(allocator)->GetTotalMemoryAllocated()) + "\n");

	logger->Log(NK::LOGGER_CHANNEL::SUCCESS, NK::LOGGER_LAYER::APPLICATION, "Engine initialised successfully!\n");

	NK_DELETE(device);

	NK::Context::Shutdown();
}