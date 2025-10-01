#pragma once
#include <RHI/IDevice.h>
#include <RHI/ICommandPool.h>

#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl.h>
#ifdef CreateSemaphore
	#undef CreateSemaphore
#endif
#include <RHI/ICommandPool.h>
#include <RHI/ICommandBuffer.h>



namespace NK
{
	class D3D12Device final : public IDevice
	{
	public:
		explicit D3D12Device(ILogger& _logger, IAllocator& _allocator);
		virtual ~D3D12Device() override;

		//IDevice interface implementation
		[[nodiscard]] virtual UniquePtr<IBuffer> CreateBuffer(const BufferDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<IBufferView> CreateBufferView(IBuffer* _buffer, const BufferViewDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<ITexture> CreateTexture(const TextureDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<ITextureView> CreateTextureView(ITexture* _texture, const TextureViewDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<ICommandPool> CreateCommandPool(const CommandPoolDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<ISurface> CreateSurface(const SurfaceDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<ISwapchain> CreateSwapchain(const SwapchainDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<IShader> CreateShader(const ShaderDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<IPipeline> CreatePipeline(const PipelineDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<IQueue> CreateQueue(const QueueDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<IFence> CreateFence(const FenceDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<ISemaphore> CreateSemaphore() override;

		//D3D12 internal API (for use by other RHI-D3D12 classes)
		[[nodiscard]] inline IDXGIFactory4* GetFactory() const { return m_factory.Get(); }
		[[nodiscard]] inline IDXGIAdapter* GetAdapter() const { return m_adapter.Get(); }
		[[nodiscard]] inline ID3D12Device* GetDevice()  const { return m_device.Get();  }
		[[nodiscard]] inline ID3D12RootSignature* GetRootSignature() const { return m_rootSig.Get(); }


	private:
		//Init sub-methods
		void EnableDebugLayer();
		void CreateFactory();
		void SelectAdapter();
		void CreateDevice();
		void RegisterCallback();
		void CreateDescriptorHeaps();
		void CreateRootSignature();
		void CreateSyncLists();

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
		std::unordered_map<COMMAND_POOL_TYPE, UniquePtr<ICommandBuffer>> m_syncLists;

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_resourceDescriptorHeap;
		UINT m_resourceDescriptorSize;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_samplerDescriptorHeap;
		UINT m_samplerDescriptorSize;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSig;


		bool m_enableDebugLayer{ true };
	};
}