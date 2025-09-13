#pragma once
#include "Core/Debug/ILogger.h"
#include "Core/Memory/Allocation.h"
#include "Core/Memory/IAllocator.h"
#include "Core/Memory/FreeListAllocator.h"


namespace NK
{
	//Forward declarations
	class IBuffer;
	struct BufferDesc;
	class IBufferView;
	struct BufferViewDesc;
	
	class ITexture;
	struct TextureDesc;
	class ITextureView;
	struct TextureViewDesc;

	class ICommandPool;
	struct CommandPoolDesc;

	class ISurface;
	struct SurfaceDesc;

	class ISwapchain;
	struct SwapchainDesc;

	class IShader;
	struct ShaderDesc;

	class IPipeline;
	struct PipelineDesc;

	
	typedef std::uint32_t ResourceIndex;
	constexpr std::uint32_t MAX_BINDLESS_RESOURCES{ 65536 };
	constexpr std::uint32_t MAX_BINDLESS_SAMPLERS{ 2048 };
}



namespace NK
{
	class IDevice
	{
	public:
		virtual ~IDevice() = default;

		[[nodiscard]] virtual UniquePtr<IBuffer> CreateBuffer(const BufferDesc& _desc) = 0;
		[[nodiscard]] virtual UniquePtr<IBufferView> CreateBufferView(IBuffer* _buffer, const BufferViewDesc& _desc) = 0;
		[[nodiscard]] virtual UniquePtr<ITexture> CreateTexture(const TextureDesc& _desc) = 0;
		[[nodiscard]] virtual UniquePtr<ITextureView> CreateTextureView(ITexture* _texture, const TextureViewDesc& _desc) = 0;
		[[nodiscard]] virtual UniquePtr<ICommandPool> CreateCommandPool(const CommandPoolDesc& _desc) = 0;
		[[nodiscard]] virtual UniquePtr<ISurface> CreateSurface(const SurfaceDesc& _desc) = 0;
		[[nodiscard]] virtual UniquePtr<ISwapchain> CreateSwapchain(const SwapchainDesc& _desc) = 0;
		[[nodiscard]] virtual UniquePtr<IShader> CreateShader(const ShaderDesc& _desc) = 0;
		[[nodiscard]] virtual UniquePtr<IPipeline> CreatePipeline(const PipelineDesc& _desc) = 0;


	protected:
		explicit IDevice(ILogger& _logger, IAllocator& _allocator)
		: m_logger(_logger), m_allocator(_allocator), m_resourceIndexAllocator(NK_NEW(FreeListAllocator, MAX_BINDLESS_RESOURCES)) {}

		//Dependency injections
		ILogger& m_logger;
		IAllocator& m_allocator;

		UniquePtr<FreeListAllocator> m_resourceIndexAllocator;
	};
}