#pragma once
#include "Core/Debug/ILogger.h"
#include "Core/Memory/Allocation.h"
#include "Core/Memory/IAllocator.h"

namespace NK
{
	//Forward declarations
	class IBuffer;
	struct BufferDesc;

	class ITexture;
	struct TextureDesc;

	class ICommandPool;
	struct CommandPoolDesc;

	class Window;
	class ISurface;

	
	typedef std::uint32_t ResourceIndex;


	enum class BUFFER_VIEW_TYPE
	{
		CONSTANT,
		SHADER_RESOURCE,
		UNORDERED_ACCESS,
	};
	
	struct BufferViewDesc
	{
		BUFFER_VIEW_TYPE type;
		std::uint64_t offset; //Offset from start of IBuffer in bytes (must be aligned to hardware specific requirements)
		std::uint64_t size; //Size of view in bytes
	};
}



namespace NK
{
	
	class IDevice
	{
	public:
		virtual ~IDevice() = default;

		[[nodiscard]] virtual UniquePtr<IBuffer> CreateBuffer(const BufferDesc& _desc) = 0;
		[[nodiscard]] virtual ResourceIndex CreateBufferView(const BufferViewDesc& _desc) = 0;
		[[nodiscard]] virtual UniquePtr<ITexture> CreateTexture(const TextureDesc& _desc) = 0;
		[[nodiscard]] virtual UniquePtr<ICommandPool> CreateCommandPool(const CommandPoolDesc& _desc) = 0;
		//[[nodiscard]] virtual UniquePtr<ISurface> CreateSurface(const Window* _window) = 0;


	protected:
		explicit IDevice(ILogger& _logger, IAllocator& _allocator) : m_logger(_logger), m_allocator(_allocator) {}

		//Dependency injections
		ILogger& m_logger;
		IAllocator& m_allocator;
	};
}