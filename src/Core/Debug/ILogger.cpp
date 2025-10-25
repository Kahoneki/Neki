#include "ILogger.h"

#include <iostream>

#if defined(_WIN32)
	#include <windows.h>
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



	std::string ILogger::LayerToString(const LOGGER_LAYER _layer)
	{
		switch (_layer)
		{
		case LOGGER_LAYER::UNKNOWN:						return "[UNKNOWN]";
		
		case LOGGER_LAYER::VULKAN_GENERAL:				return "[VULKAN GENERAL]";
		case LOGGER_LAYER::VULKAN_VALIDATION:			return "[VULKAN VALIDATION]";
		case LOGGER_LAYER::VULKAN_PERFORMANCE:			return "[VULKAN PERFORMANCE]";
		
		case LOGGER_LAYER::D3D12_APP_DEFINED:			return "[D3D12 APP DEFINED]";
		case LOGGER_LAYER::D3D12_MISC:					return "[D3D12 MISC]";
		case LOGGER_LAYER::D3D12_INIT:					return "[D3D12 INIT]";
		case LOGGER_LAYER::D3D12_CLEANUP:				return "[D3D12 CLEANUP]";
		case LOGGER_LAYER::D3D12_COMPILATION:			return "[D3D12 COMPILATION]";
		case LOGGER_LAYER::D3D12_STATE_CREATE:			return "[D3D12 STATE CREATE]";
		case LOGGER_LAYER::D3D12_STATE_SET:				return "[D3D12 STATE SET]";
		case LOGGER_LAYER::D3D12_STATE_GET:				return "[D3D12 STATE GET]";
		case LOGGER_LAYER::D3D12_RES_MANIP:				return "[D3D12 RES MANIP]";
		case LOGGER_LAYER::D3D12_EXECUTION:				return "[D3D12 EXECUTION]";
		case LOGGER_LAYER::D3D12_SHADER:				return "[D3D12 SHADER]";
		
		case LOGGER_LAYER::GLFW:						return "[GLFW]";
		
		case LOGGER_LAYER::ENGINE:						return "[ENGINE]";
		case LOGGER_LAYER::CONTEXT:						return "[CONTEXT]";
		case LOGGER_LAYER::TRACKING_ALLOCATOR:			return "[TRACKING ALLOCATOR]";

		case LOGGER_LAYER::RENDER_LAYER:				return "[RENDER LAYER]";
		case LOGGER_LAYER::CLIENT_NETWORK_LAYER:		return "[CLIENT NETWORK LAYER]";
		case LOGGER_LAYER::SERVER_NETWORK_LAYER:		return "[SERVER NETWORK LAYER]";
		case LOGGER_LAYER::INPUT_LAYER:					return "[INPUT LAYER]";
		case LOGGER_LAYER::PLAYER_CAMERA_LAYER:			return "[PLAYER CAMERA LAYER]";
		case LOGGER_LAYER::DISK_MODEL_LOADER_LAYER:		return "[DISK MODEL LOADER LAYER]";
			
		case LOGGER_LAYER::DEVICE:						return "[DEVICE]";
		case LOGGER_LAYER::COMMAND_POOL:				return "[COMMAND POOL]";
		case LOGGER_LAYER::COMMAND_BUFFER:				return "[COMMAND BUFFER]";
		case LOGGER_LAYER::BUFFER:						return "[BUFFER]";
		case LOGGER_LAYER::BUFFER_VIEW:					return "[BUFFER VIEW]";
		case LOGGER_LAYER::TEXTURE:						return "[TEXTURE]";
		case LOGGER_LAYER::TEXTURE_VIEW:				return "[TEXTURE VIEW]";
		case LOGGER_LAYER::SAMPLER:						return "[SAMPLER]";
		case LOGGER_LAYER::SURFACE:						return "[SURFACE]";
		case LOGGER_LAYER::SWAPCHAIN:					return "[SWAPCHAIN]";
		case LOGGER_LAYER::SHADER:						return "[SHADER]";
		case LOGGER_LAYER::PIPELINE:					return "[PIPELINE]";
		case LOGGER_LAYER::ROOT_SIGNATURE:				return "[ROOT SIGNATURE]";
		case LOGGER_LAYER::QUEUE:						return "[QUEUE]";
		case LOGGER_LAYER::FENCE:						return "[FENCE]";
		case LOGGER_LAYER::SEMAPHORE:					return "[SEMAPHORE]";

		case LOGGER_LAYER::GPU_UPLOADER:				return "[GPU UPLOADER]";
		case LOGGER_LAYER::WINDOW:						return "[WINDOW]";
		
		case LOGGER_LAYER::APPLICATION:					return "[APPLICATION]";

		default:
		{
			throw std::runtime_error("ILogger::LayerToString() returned default case - _layer = " + std::to_string(std::to_underlying(_layer)));
		}
		}
	}

}