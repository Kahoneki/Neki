#pragma once
#include "IDevice.h"
#include "Core/Debug/ILogger.h"
#include "Core/Memory/Allocation.h"
#include "Core/Memory/IAllocator.h"

namespace NK
{
	class IBuffer;
	struct BufferDesc;

	class ITexture;
	struct TextureDesc;

	class ICommandPool;
	struct CommandPoolDesc;

	class Window;
	class ISurface;
}



namespace NK
{

	class IDevice
	{
	public:
		virtual ~IDevice() = default;

		[[nodiscard]] virtual UniquePtr<IBuffer> CreateBuffer(const BufferDesc& _desc) = 0;
//		[[nodiscard]] virtual UniquePtr<ITexture> CreateTexture(const TextureDesc& _desc) = 0;
		[[nodiscard]] virtual UniquePtr<ICommandPool> CreateCommandPool(const CommandPoolDesc& _desc) = 0;
//		[[nodiscard]] virtual UniquePtr<ISurface> CreateSurface(const Window* _window) = 0;


	protected:
		explicit IDevice(ILogger& _logger, IAllocator& _allocator) : m_logger(_logger), m_allocator(_allocator) {}

		//Dependency injections
		ILogger& m_logger;
		IAllocator& m_allocator;
	};

}