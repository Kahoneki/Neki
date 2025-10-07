#pragma once

#include <cstdint>
#include <unordered_map>
#include "../Utils/enum_enable_bitmask_operators.h"

namespace NK
{
	//Defines the severity of a log message
	enum class LOGGER_CHANNEL : std::uint32_t
	{
		NONE	= 0,
		HEADING = 1 << 0,
		INFO	= 1 << 1,
		WARNING = 1 << 2,
		ERROR   = 1 << 3,
		SUCCESS = 1 << 4,
	};

}

//Enable bitmask operators for the LOGGER_CHANNEL type
template<>
struct enable_bitmask_operators<NK::LOGGER_CHANNEL> : std::true_type {};


namespace NK
{

	enum class LOGGER_LAYER : std::uint32_t
	{
		UNKNOWN,

		VULKAN_GENERAL,
		VULKAN_VALIDATION,
		VULKAN_PERFORMANCE,

		D3D12_APP_DEFINED,
		D3D12_MISC,
		D3D12_INIT,
		D3D12_CLEANUP,
		D3D12_COMPILATION,
		D3D12_STATE_CREATE,
		D3D12_STATE_SET,
		D3D12_STATE_GET,
		D3D12_RES_MANIP,
		D3D12_EXECUTION,
		D3D12_SHADER,

		CONTEXT,
		TRACKING_ALLOCATOR,
		
		DEVICE,
		COMMAND_POOL,
		COMMAND_BUFFER,
		BUFFER,
		BUFFER_VIEW,
		TEXTURE,
		TEXTURE_VIEW,
		SAMPLER,
		SURFACE,
		SWAPCHAIN,
		SHADER,
		ROOT_SIGNATURE,
		PIPELINE,
		QUEUE,
		FENCE,
		SEMAPHORE,

		GPU_UPLOADER,
		WINDOW,
		
		APPLICATION,
	};


	enum class LOGGER_TYPE
	{
		CONSOLE,
	};


	class LoggerConfig final
	{
	public:
		explicit LoggerConfig(LOGGER_TYPE _type, bool _enableAll = false);
		~LoggerConfig() = default;

		//Set the default logging channels for all layers that have not been explicitly set
		void SetDefaultChannelBitfield(LOGGER_CHANNEL _channels);

		//Set the logging channels for a specific layer
		void SetLayerChannelBitfield(LOGGER_LAYER _layer, LOGGER_CHANNEL _channels);

		//Get the logging channels for a specific layer
		[[nodiscard]] LOGGER_CHANNEL GetChannelBitfieldForLayer(LOGGER_LAYER _layer) const;

		const LOGGER_TYPE type;


	private:
		LOGGER_CHANNEL m_defaultChannelBitfield;
		std::unordered_map<LOGGER_LAYER, LOGGER_CHANNEL> m_layerChannelBitfield;
	};
}