#pragma once

#include <Core/Debug/ILogger.h>
#include <Core/Memory/Allocation.h>
#include <Core/Memory/FreeListAllocator.h>
#include <Core/Memory/IAllocator.h>


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

	class ISampler;
	struct SamplerDesc;

	class ICommandPool;
	struct CommandPoolDesc;

	class ISurface;

	class ISwapchain;
	struct SwapchainDesc;

	class IShader;
	struct ShaderDesc;

	class IRootSignature;
	struct RootSignatureDesc;

	class IPipeline;
	struct PipelineDesc;

	class IQueue;
	struct QueueDesc;

	class IFence;
	struct FenceDesc;

	class ISemaphore;

	class GPUUploader;
	struct GPUUploaderDesc;

	class Window;
	struct WindowDesc;

	
	constexpr std::uint32_t MAX_BINDLESS_RESOURCES{ 65536 };
	constexpr std::uint32_t MAX_BINDLESS_SAMPLERS{ 2048 };
	
	
	class IDevice
	{
	public:
		virtual ~IDevice() = default;

		[[nodiscard]] virtual UniquePtr<IBuffer> CreateBuffer(const BufferDesc& _desc) = 0;
		[[nodiscard]] virtual UniquePtr<IBufferView> CreateBufferView(IBuffer* _buffer, const BufferViewDesc& _desc) = 0;
		[[nodiscard]] virtual UniquePtr<ITexture> CreateTexture(const TextureDesc& _desc) = 0;
		//Can be used to create shader read-only texture views and shader read-write texture views
		[[nodiscard]] virtual UniquePtr<ITextureView> CreateShaderResourceTextureView(ITexture* _texture, const TextureViewDesc& _desc) = 0;
		//Can be used to create depth texture views, stencil texture views, or depth-stencil texture views
		[[nodiscard]] virtual UniquePtr<ITextureView> CreateDepthStencilTextureView(ITexture* _texture, const TextureViewDesc& _desc) = 0;
		//Can be used to create render target views
		[[nodiscard]] virtual UniquePtr<ITextureView> CreateRenderTargetTextureView(ITexture* _texture, const TextureViewDesc& _desc) = 0;
		[[nodiscard]] virtual UniquePtr<ISampler> CreateSampler(const SamplerDesc& _desc) = 0;
		[[nodiscard]] virtual UniquePtr<ICommandPool> CreateCommandPool(const CommandPoolDesc& _desc) = 0;
		[[nodiscard]] virtual UniquePtr<ISurface> CreateSurface(Window* _window) = 0;
		[[nodiscard]] virtual UniquePtr<ISwapchain> CreateSwapchain(const SwapchainDesc& _desc) = 0;
		[[nodiscard]] virtual UniquePtr<IShader> CreateShader(const ShaderDesc& _desc) = 0;
		[[nodiscard]] virtual UniquePtr<IRootSignature> CreateRootSignature(const RootSignatureDesc& _desc) = 0;
		[[nodiscard]] virtual UniquePtr<IPipeline> CreatePipeline(const PipelineDesc& _desc) = 0;
		[[nodiscard]] virtual UniquePtr<IQueue> CreateQueue(const QueueDesc& _desc) = 0;
		[[nodiscard]] virtual UniquePtr<IFence> CreateFence(const FenceDesc& _desc) = 0;
		[[nodiscard]] virtual UniquePtr<ISemaphore> CreateSemaphore() = 0;
		[[nodiscard]] virtual UniquePtr<GPUUploader> CreateGPUUploader(const GPUUploaderDesc& _desc) = 0;
		[[nodiscard]] virtual UniquePtr<Window> CreateWindow(const WindowDesc& _desc) const = 0;

		//When copying data from a buffer to a texture, the buffer data needs to be in a specific layout that depends on the destination texture
		//This function calculates it for you
		[[nodiscard]] virtual TextureCopyMemoryLayout GetRequiredMemoryLayoutForTextureCopy(ITexture* _texture) = 0;


	protected:
		explicit IDevice(ILogger& _logger, IAllocator& _allocator)
		: m_logger(_logger), m_allocator(_allocator),
		m_resourceIndexAllocator(NK_NEW(FreeListAllocator, MAX_BINDLESS_RESOURCES)), m_samplerIndexAllocator(NK_NEW(FreeListAllocator, MAX_BINDLESS_SAMPLERS)) {}

		//Dependency injections
		ILogger& m_logger;
		IAllocator& m_allocator;

		UniquePtr<FreeListAllocator> m_resourceIndexAllocator;
		UniquePtr<FreeListAllocator> m_samplerIndexAllocator;
	};
	
}