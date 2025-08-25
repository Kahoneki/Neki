#pragma once
#include "IDevice.h"
#include "Core/Debug/ILogger.h"
#include "Core/Memory/IAllocator.h"


namespace NK
{
	struct CommandBufferDesc;
	class ICommandBuffer;
}


namespace NK
{
	enum class COMMAND_POOL_TYPE : std::uint32_t
	{
		GRAPHICS,
		COMPUTE,
		TRANSFER,
	};

	struct CommandPoolDesc
	{
		COMMAND_POOL_TYPE type;
	};


	enum class COMMAND_POOL_RESET_FLAGS : std::uint32_t
	{
		NONE,
		RELEASE_RESOURCES,
	};


	class ICommandPool
	{
	public:
		virtual ~ICommandPool() = default;

		[[nodiscard]] virtual UniquePtr<ICommandBuffer> AllocateCommandBuffer(const CommandBufferDesc& _desc) = 0;
		virtual void Reset(COMMAND_POOL_RESET_FLAGS _type) = 0;


	protected:
		explicit ICommandPool(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const CommandPoolDesc& _desc)
		: m_logger(_logger), m_allocator(_allocator), m_device(_device), m_type(_desc.type) {}

		//Dependency injections
		ILogger& m_logger;
		IAllocator& m_allocator;
		IDevice& m_device;
		
		COMMAND_POOL_TYPE m_type;
	};
}