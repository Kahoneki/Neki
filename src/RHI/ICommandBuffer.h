#pragma once
#include "ICommandPool.h"

namespace NK
{
	enum class COMMAND_BUFFER_LEVEL
	{
		PRIMARY,
		SECONDARY,
	};

	struct CommandBufferDesc
	{
		COMMAND_BUFFER_LEVEL level;
	};


	class ICommandBuffer
	{
	public:
		virtual ~ICommandBuffer();

		//todo: add command buffer methods here
		virtual void Reset() = 0;


	protected:
		explicit ICommandBuffer(ILogger& _logger, IDevice& _device, ICommandPool& _pool, const CommandBufferDesc& _desc)
		: m_logger(_logger), m_device(_device), m_pool(_pool), m_level(_desc.level) {}



		//Dependency injections
		ILogger& m_logger;
		IDevice& m_device;
		ICommandPool& m_pool;

		COMMAND_BUFFER_LEVEL m_level;
	};
}