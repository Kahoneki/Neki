#pragma once

#include <RHI/ICommandBuffer.h>
#include <RHI/ICommandPool.h>
#include <RHI/IDevice.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#ifdef CreateSemaphore
	#undef CreateSemaphore
#endif
#ifdef CreateWindow
	#undef CreateWindow
#endif
#if defined(ERROR)
	#undef ERROR //Error is used for LOGGER_CHANNEL::ERROR
#endif
#ifdef LoadImage
	#undef LoadImage //Conflicts with ImageLoader::LoadImage()
#endif


namespace NK
{
	constexpr std::uint32_t MAX_DEPTH_STENCIL_VIEWS{ 2048 };
	constexpr std::uint32_t MAX_RENDER_TARGET_VIEWS{ 2048 };


	class D3D12Device final : public IDevice
	{
	public:
		explicit D3D12Device(ILogger& _logger, IAllocator& _allocator);
		virtual ~D3D12Device() override;

		//IDevice interface implementation
		[[nodiscard]] virtual UniquePtr<IBuffer> CreateBuffer(const BufferDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<IBufferView> CreateBufferView(IBuffer* _buffer, const BufferViewDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<ITexture> CreateTexture(const TextureDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<ITextureView> CreateShaderResourceTextureView(ITexture* _texture, const TextureViewDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<ITextureView> CreateDepthStencilTextureView(ITexture* _texture, const TextureViewDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<ITextureView> CreateRenderTargetTextureView(ITexture* _texture, const TextureViewDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<ISampler> CreateSampler(const SamplerDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<ICommandPool> CreateCommandPool(const CommandPoolDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<ISurface> CreateSurface(Window* _window) override;
		[[nodiscard]] virtual UniquePtr<ISwapchain> CreateSwapchain(const SwapchainDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<IShader> CreateShader(const ShaderDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<IRootSignature> CreateRootSignature(const RootSignatureDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<IPipeline> CreatePipeline(const PipelineDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<IQueue> CreateQueue(const QueueDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<IFence> CreateFence(const FenceDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<ISemaphore> CreateSemaphore() override;
		[[nodiscard]] virtual UniquePtr<GPUUploader> CreateGPUUploader(const GPUUploaderDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<Window> CreateWindow(const WindowDesc& _desc) const override;

		[[nodiscard]] virtual TextureCopyMemoryLayout GetRequiredMemoryLayoutForTextureCopy(ITexture* _texture) override;

		//D3D12 internal API (for use by other RHI-D3D12 classes)
		[[nodiscard]] inline IDXGIFactory4* GetFactory() const { return m_factory.Get(); }
		[[nodiscard]] inline IDXGIAdapter* GetAdapter() const { return m_adapter.Get(); }
		[[nodiscard]] inline ID3D12Device* GetDevice() const { return m_device.Get();  }
		[[nodiscard]] inline ID3D12DescriptorHeap* GetResourceDescriptorHeap() const { return m_resourceDescriptorHeap.Get(); }
		[[nodiscard]] inline ID3D12DescriptorHeap* GetSamplerDescriptorHeap() const { return m_samplerDescriptorHeap.Get(); }
		[[nodiscard]] inline UINT GetResourceDescriptorSize() const { return m_resourceDescriptorSize; }
		[[nodiscard]] inline UINT GetSamplerDescriptorSize() const { return m_samplerDescriptorSize; }


	private:
		//Init sub-methods
		void EnableDebugLayer();
		void CreateFactory();
		void SelectAdapter();
		void CreateDevice();
		void RegisterDebugCallback();
		void CreateDescriptorHeaps();
		void CreateSyncLists();

		//Shutdown sub-methods
		void UnregisterDebugCallback();

		static void DebugCallback(D3D12_MESSAGE_CATEGORY _category, D3D12_MESSAGE_SEVERITY _severity, D3D12_MESSAGE_ID _id, LPCSTR _pDescription, void* _pContext);


		const D3D_FEATURE_LEVEL m_featureLevel{ D3D_FEATURE_LEVEL_12_0 };

		//D3D12 handles
		Microsoft::WRL::ComPtr<IDXGIFactory4> m_factory;
		Microsoft::WRL::ComPtr<IDXGIAdapter1> m_adapter;
		Microsoft::WRL::ComPtr<ID3D12Device> m_device;

		//For passing to D3D12Queue for D3D12Queue::WaitIdle()
		UniquePtr<ICommandPool> m_graphicsSyncListAllocator;
		UniquePtr<ICommandPool> m_computeSyncListAllocator;
		UniquePtr<ICommandPool> m_transferSyncListAllocator;
		std::unordered_map<COMMAND_TYPE, UniquePtr<ICommandBuffer>> m_syncLists;

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_resourceDescriptorHeap;
		UINT m_resourceDescriptorSize;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_samplerDescriptorHeap;
		UINT m_samplerDescriptorSize;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvDescriptorHeap;
		UINT m_dsvDescriptorSize;
		UniquePtr<FreeListAllocator> m_dsvIndexAllocator;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
		UINT m_rtvDescriptorSize;
		UniquePtr<FreeListAllocator> m_rtvIndexAllocator;


		bool m_enableDebugLayer{ true };
		DWORD m_debugCallbackFuncCookie;
	};
}