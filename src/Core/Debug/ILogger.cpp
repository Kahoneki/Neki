#include "ILogger.h"
#include <iomanip>
#include <iostream>

#if defined(_WIN32)
	#include <windows.h>
	#if defined(ERROR)
		#undef ERROR //Error is used for LOGGER_CHANNEL::ERROR
	#endif
#endif

namespace NK
{

	ILogger::ILogger(const LoggerConfig& _config)
	: m_config(_config)
	{
		if (!EnableAnsiSupport())
		{
			std::cerr << "ERR::Logger::Logger::FAILED_TO_ENABLE_ANSI_SUPPORT::FALLING_BACK_TO_DEFAULT_CHANNEL" << std::endl;
		}
	}



	bool ILogger::EnableAnsiSupport()
	{
		#if defined(_WIN32)
		HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		if (hOut == INVALID_HANDLE_VALUE) { return false; }
		DWORD dwMode = 0;
		if (!GetConsoleMode(hOut, &dwMode)) { return false; }
		dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		if (!SetConsoleMode(hOut, dwMode)) { return false; }
		#endif

		return true;
	}



	std::string ILogger::LayerToString(LOGGER_LAYER _layer)
	{
		switch (_layer)
		{
		case LOGGER_LAYER::VULKAN_GENERAL:		return "[VULKAN GENERAL]";
		case LOGGER_LAYER::VULKAN_VALIDATION:	return "[VULKAN VALIDATION]";
		case LOGGER_LAYER::VULKAN_PERFORMANCE:	return "[VULKAN PERFORMANCE]";

		case LOGGER_LAYER::CONTEXT:				return "[CONTEXT]";
		case LOGGER_LAYER::TRACKING_ALLOCATOR:	return "[TRACKING ALLOCATOR]";
			
		case LOGGER_LAYER::DEVICE:				return "[DEVICE]";
		case LOGGER_LAYER::COMMAND_POOL:		return "[COMMAND POOL]";
		case LOGGER_LAYER::COMMAND_BUFFER:		return "[COMMAND BUFFER]";
		case LOGGER_LAYER::BUFFER:				return "[BUFFER]";
		case LOGGER_LAYER::TEXTURE:				return "[TEXTURE]";
			
		case LOGGER_LAYER::SWAPCHAIN:			return "[SWAPCHAIN]";
		case LOGGER_LAYER::RENDER_MANAGER:		return "[RENDER MANAGER]";
		case LOGGER_LAYER::DESCRIPTOR_POOL:		return "[DESCRIPTOR POOL]";
		case LOGGER_LAYER::PIPELINE:			return "[PIPELINE]";
		case LOGGER_LAYER::MODEL_FACTORY:		return "[MODEL FACTORY]";
		case LOGGER_LAYER::APPLICATION:			return "[APPLICATION]";

		default:								return "[UNDEFINED]";
		}
	}

}